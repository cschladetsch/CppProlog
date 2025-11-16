#include "unification.hpp"
#include <format>
#include <sstream>
#include <atomic>

namespace logicpp {

// Substitution implementation
Substitution::Substitution(const Substitution& other) {
    for (const auto& [var, term] : other.bindings) {
        bindings[var] = term_utils::deep_copy(*term);
    }
}

Substitution& Substitution::operator=(const Substitution& other) {
    if (this != &other) {
        bindings.clear();
        for (const auto& [var, term] : other.bindings) {
            bindings[var] = term_utils::deep_copy(*term);
        }
    }
    return *this;
}

void Substitution::bind(const std::string& variable, std::unique_ptr<ast::Term> term) {
    bindings[variable] = std::move(term);
}

const ast::Term* Substitution::lookup(const std::string& variable) const {
    auto it = bindings.find(variable);
    return (it != bindings.end()) ? it->second.get() : nullptr;
}

bool Substitution::is_bound(const std::string& variable) const {
    return bindings.find(variable) != bindings.end();
}

std::unique_ptr<ast::Term> Substitution::apply(const ast::Term& term) const {
    if (auto var = dynamic_cast<const ast::Variable*>(&term)) {
        if (auto bound_term = lookup(var->name)) {
            // Recursively apply substitution to avoid chains
            return apply(*bound_term);
        }
        return term_utils::deep_copy(term);
    }
    
    if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        auto new_compound = std::make_unique<ast::Compound>(
            compound->position, compound->functor);
        
        for (const auto& arg : compound->arguments) {
            new_compound->arguments.push_back(apply(*arg));
        }
        
        return std::move(new_compound);
    }
    
    // For atoms, numbers, strings - just copy
    return term_utils::deep_copy(term);
}

Substitution Substitution::compose(const Substitution& other) const {
    Substitution result = *this;
    
    // Apply this substitution to other's bindings and add them
    for (const auto& [var, term] : other.bindings) {
        if (!result.is_bound(var)) {
            result.bind(var, apply(*term));
        }
    }
    
    // Apply other's substitution to our existing bindings
    for (auto& [var, term] : result.bindings) {
        term = other.apply(*term);
    }
    
    return result;
}

std::vector<std::string> Substitution::variables() const {
    std::vector<std::string> vars;
    vars.reserve(bindings.size());
    
    for (const auto& [var, _] : bindings) {
        vars.push_back(var);
    }
    
    return vars;
}

std::string Substitution::to_string() const {
    if (bindings.empty()) return "{}";
    
    std::ostringstream oss;
    oss << "{ ";
    bool first = true;
    
    for (const auto& [var, term] : bindings) {
        if (!first) oss << ", ";
        oss << var << " -> " << term_utils::term_to_string(*term);
        first = false;
    }
    
    oss << " }";
    return oss.str();
}

// UnificationEngine implementation
UnificationResult UnificationEngine::unify(const ast::Term& term1, const ast::Term& term2) const {
    Substitution substitution;
    return unify(term1, term2, substitution);
}

UnificationResult UnificationEngine::unify(
    const ast::Term& term1, 
    const ast::Term& term2,
    const Substitution& existing_substitution) const {
    
    Substitution substitution = existing_substitution;
    
    // Apply existing substitution first
    auto applied_term1 = substitution.apply(term1);
    auto applied_term2 = substitution.apply(term2);
    
    // Recursively unify the applied terms
    if (auto var1 = dynamic_cast<const ast::Variable*>(applied_term1.get())) {
        if (auto var2 = dynamic_cast<const ast::Variable*>(applied_term2.get())) {
            return unify_variables(*var1, *var2, substitution);
        } else {
            return unify_variable_with_term(*var1, *applied_term2, substitution);
        }
    }
    
    if (auto var2 = dynamic_cast<const ast::Variable*>(applied_term2.get())) {
        return unify_variable_with_term(*var2, *applied_term1, substitution);
    }
    
    // Handle other cases similar to unify_internal logic
    if (auto comp1 = dynamic_cast<const ast::Compound*>(applied_term1.get())) {
        if (auto comp2 = dynamic_cast<const ast::Compound*>(applied_term2.get())) {
            return unify_compounds(*comp1, *comp2, substitution);
        }
        return UnificationResult::failure();
    }
    
    if (auto atom1 = dynamic_cast<const ast::Atom*>(applied_term1.get())) {
        if (auto atom2 = dynamic_cast<const ast::Atom*>(applied_term2.get())) {
            auto result = unify_atoms(*atom1, *atom2);
            if (result.success) {
                result.substitution = std::move(substitution);
            }
            return result;
        }
        return UnificationResult::failure();
    }
    
    // Similar logic for numbers and strings...
    return UnificationResult::failure();
}


UnificationResult UnificationEngine::unify_variables(
    const ast::Variable& var1, 
    const ast::Variable& var2,
    Substitution& substitution) const {
    
    // Anonymous variables unify with anything
    if (var1.name == "_" || var2.name == "_") {
        return UnificationResult::success(std::move(substitution));
    }
    
    // Same variable
    if (var1.name == var2.name) {
        return UnificationResult::success(std::move(substitution));
    }
    
    // Check if either is already bound
    if (const ast::Term* bound1 = substitution.lookup(var1.name)) {
        return unify(*bound1, var2, substitution);
    }
    
    if (const ast::Term* bound2 = substitution.lookup(var2.name)) {
        return unify(var1, *bound2, substitution);
    }
    
    // Bind var1 to var2 (arbitrary choice)
    substitution.bind(var1.name, std::make_unique<ast::Variable>(var2.position, var2.name));
    return UnificationResult::success(std::move(substitution));
}

UnificationResult UnificationEngine::unify_variable_with_term(
    const ast::Variable& variable,
    const ast::Term& term,
    Substitution& substitution) const {
    
    // Anonymous variable unifies with anything
    if (variable.name == "_") {
        return UnificationResult::success(std::move(substitution));
    }
    
    // Check if variable is already bound
    if (const ast::Term* bound_term = substitution.lookup(variable.name)) {
        return unify(*bound_term, term, substitution);
    }
    
    // Occur check to prevent infinite structures
    if (occurs_check(variable.name, term)) {
        return UnificationResult::failure();
    }
    
    // Bind variable to term
    substitution.bind(variable.name, term_utils::deep_copy(term));
    return UnificationResult::success(std::move(substitution));
}

UnificationResult UnificationEngine::unify_compounds(
    const ast::Compound& comp1,
    const ast::Compound& comp2,
    Substitution& substitution) const {
    
    // Different functors
    if (comp1.functor != comp2.functor) {
        return UnificationResult::failure();
    }
    
    // Different arity
    if (comp1.arguments.size() != comp2.arguments.size()) {
        return UnificationResult::failure();
    }
    
    // Unify arguments pairwise
    return unify_lists(comp1.arguments, comp2.arguments, substitution);
}

UnificationResult UnificationEngine::unify_atoms(
    const ast::Atom& atom1,
    const ast::Atom& atom2) const {
    
    if (atom1.value == atom2.value) {
        return UnificationResult::success();
    }
    return UnificationResult::failure();
}

UnificationResult UnificationEngine::unify_numbers(
    const ast::Number& num1,
    const ast::Number& num2) const {
    
    // Handle different number types
    if (std::holds_alternative<int64_t>(num1.value) && 
        std::holds_alternative<int64_t>(num2.value)) {
        if (std::get<int64_t>(num1.value) == std::get<int64_t>(num2.value)) {
            return UnificationResult::success();
        }
    } else if (std::holds_alternative<double>(num1.value) && 
               std::holds_alternative<double>(num2.value)) {
        if (std::get<double>(num1.value) == std::get<double>(num2.value)) {
            return UnificationResult::success();
        }
    }
    // Could add mixed int/double comparison here
    
    return UnificationResult::failure();
}

UnificationResult UnificationEngine::unify_strings(
    const ast::String& str1,
    const ast::String& str2) const {
    
    if (str1.value == str2.value) {
        return UnificationResult::success();
    }
    return UnificationResult::failure();
}

UnificationResult UnificationEngine::unify_lists(
    const std::vector<std::unique_ptr<ast::Term>>& terms1,
    const std::vector<std::unique_ptr<ast::Term>>& terms2,
    Substitution& substitution) const {
    
    if (terms1.size() != terms2.size()) {
        return UnificationResult::failure();
    }
    
    for (size_t i = 0; i < terms1.size(); ++i) {
        auto result = unify(*terms1[i], *terms2[i], substitution);
        if (!result.success) {
            return result;
        }
        substitution = std::move(result.substitution);
    }
    
    return UnificationResult::success(std::move(substitution));
}

bool UnificationEngine::occurs_check(const std::string& variable, const ast::Term& term) const {
    if (auto var = dynamic_cast<const ast::Variable*>(&term)) {
        return var->name == variable;
    }
    
    if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        for (const auto& arg : compound->arguments) {
            if (occurs_check(variable, *arg)) {
                return true;
            }
        }
    }
    
    return false;
}

bool UnificationEngine::unifiable(const ast::Term& term1, const ast::Term& term2) const {
    return unify(term1, term2).success;
}

std::string UnificationEngine::fresh_variable_name(const std::string& base) const {
    static std::atomic<size_t> counter{0};
    return std::format("{}_{}", base, counter.fetch_add(1));
}

// Term utilities implementation
namespace term_utils {

bool is_variable(const ast::Term& term) {
    return dynamic_cast<const ast::Variable*>(&term) != nullptr;
}

bool is_ground(const ast::Term& term) {
    if (is_variable(term)) {
        return false;
    }
    
    if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        for (const auto& arg : compound->arguments) {
            if (!is_ground(*arg)) {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<std::string> get_variables(const ast::Term& term) {
    std::vector<std::string> variables;
    
    if (auto var = dynamic_cast<const ast::Variable*>(&term)) {
        if (var->name != "_") {  // Exclude anonymous variables
            variables.push_back(var->name);
        }
    } else if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        for (const auto& arg : compound->arguments) {
            auto arg_vars = get_variables(*arg);
            variables.insert(variables.end(), arg_vars.begin(), arg_vars.end());
        }
    }
    
    // Remove duplicates
    std::sort(variables.begin(), variables.end());
    variables.erase(std::unique(variables.begin(), variables.end()), variables.end());
    
    return variables;
}

std::string term_to_string(const ast::Term& term) {
    if (auto var = dynamic_cast<const ast::Variable*>(&term)) {
        return var->name;
    }
    
    if (auto atom = dynamic_cast<const ast::Atom*>(&term)) {
        return atom->value;
    }
    
    if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        if (compound->arguments.empty()) {
            return compound->functor;
        }
        
        std::ostringstream oss;
        oss << compound->functor << "(";
        for (size_t i = 0; i < compound->arguments.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << term_to_string(*compound->arguments[i]);
        }
        oss << ")";
        return oss.str();
    }
    
    if (auto number = dynamic_cast<const ast::Number*>(&term)) {
        if (std::holds_alternative<int64_t>(number->value)) {
            return std::to_string(std::get<int64_t>(number->value));
        } else {
            return std::to_string(std::get<double>(number->value));
        }
    }
    
    if (auto str = dynamic_cast<const ast::String*>(&term)) {
        return "\"" + str->value + "\"";
    }
    
    return "<?>";
}

std::unique_ptr<ast::Term> deep_copy(const ast::Term& term) {
    if (auto var = dynamic_cast<const ast::Variable*>(&term)) {
        return std::make_unique<ast::Variable>(var->position, var->name);
    }
    
    if (auto atom = dynamic_cast<const ast::Atom*>(&term)) {
        return std::make_unique<ast::Atom>(atom->position, atom->value);
    }
    
    if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        auto new_compound = std::make_unique<ast::Compound>(
            compound->position, compound->functor);
        
        for (const auto& arg : compound->arguments) {
            new_compound->arguments.push_back(deep_copy(*arg));
        }
        
        return std::move(new_compound);
    }
    
    if (auto number = dynamic_cast<const ast::Number*>(&term)) {
        if (std::holds_alternative<int64_t>(number->value)) {
            return std::make_unique<ast::Number>(
                number->position, std::get<int64_t>(number->value));
        } else {
            return std::make_unique<ast::Number>(
                number->position, std::get<double>(number->value));
        }
    }
    
    if (auto str = dynamic_cast<const ast::String*>(&term)) {
        return std::make_unique<ast::String>(str->position, str->value);
    }
    
    // Fallback
    return std::make_unique<ast::Atom>(Position{}, "<?>");
}

} // namespace term_utils

} // namespace logicpp