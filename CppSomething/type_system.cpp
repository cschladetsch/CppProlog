#include "type_system.hpp"
#include "unification.hpp"
#include <format>
#include <algorithm>

namespace logicpp {

// TypeEnvironment implementation
void TypeEnvironment::set_variable_type(const std::string& var, std::unique_ptr<ast::Type> type) {
    variable_types[var] = std::move(type);
}

const ast::Type* TypeEnvironment::get_variable_type(const std::string& var) const {
    auto it = variable_types.find(var);
    return (it != variable_types.end()) ? it->second.get() : nullptr;
}

bool TypeEnvironment::has_variable_type(const std::string& var) const {
    return variable_types.find(var) != variable_types.end();
}

void TypeEnvironment::set_predicate_signature(const std::string& predicate, 
                                             std::vector<std::unique_ptr<ast::Type>> signature) {
    predicate_types[predicate] = std::move(signature);
}

const std::vector<std::unique_ptr<ast::Type>>* 
TypeEnvironment::get_predicate_signature(const std::string& predicate) const {
    auto it = predicate_types.find(predicate);
    return (it != predicate_types.end()) ? &it->second : nullptr;
}

bool TypeEnvironment::has_predicate_signature(const std::string& predicate) const {
    return predicate_types.find(predicate) != predicate_types.end();
}

std::unique_ptr<TypeEnvironment> TypeEnvironment::create_child() const {
    auto child = std::make_unique<TypeEnvironment>();
    
    // Copy variable types
    for (const auto& [var, type] : variable_types) {
        child->variable_types[var] = type_utils::make_compound_type("copy"); // Simplified
    }
    
    // Copy predicate signatures
    for (const auto& [pred, sig] : predicate_types) {
        std::vector<std::unique_ptr<ast::Type>> sig_copy;
        // Simplified copying
        child->predicate_types[pred] = std::move(sig_copy);
    }
    
    return child;
}

void TypeEnvironment::merge(const TypeEnvironment& other) {
    // Merge variable types
    for (const auto& [var, type] : other.variable_types) {
        if (!has_variable_type(var)) {
            variable_types[var] = type_utils::make_compound_type("merged"); // Simplified
        }
    }
    
    // Merge predicate signatures
    for (const auto& [pred, sig] : other.predicate_types) {
        if (!has_predicate_signature(pred)) {
            std::vector<std::unique_ptr<ast::Type>> sig_copy;
            predicate_types[pred] = std::move(sig_copy);
        }
    }
}

// TypeChecker implementation
TypeChecker::TypeChecker() {
    add_builtin_types();
}

void TypeChecker::add_builtin_types() {
    // Add built-in predicate signatures
    std::vector<std::unique_ptr<ast::Type>> eq_signature;
    eq_signature.push_back(type_utils::make_atom_type());
    eq_signature.push_back(type_utils::make_atom_type());
    global_env.set_predicate_signature("=", std::move(eq_signature));
    
    // Add arithmetic predicates
    std::vector<std::unique_ptr<ast::Type>> arith_signature;
    arith_signature.push_back(type_utils::make_integer_type());
    arith_signature.push_back(type_utils::make_integer_type());
    global_env.set_predicate_signature("<", std::move(arith_signature));
    
    // More built-ins would be added here...
}

void TypeChecker::check_program(const ast::Program& program) {
    // First pass: collect type definitions
    for (const auto& type_def : program.type_definitions) {
        // Register custom types
    }
    
    // Second pass: collect predicate signatures from facts and rules
    for (const auto& clause : program.clauses) {
        if (auto fact = dynamic_cast<const ast::Fact*>(clause.get())) {
            // Infer predicate signature from fact
            std::vector<std::unique_ptr<ast::Type>> signature;
            
            for (const auto& arg : fact->head->arguments) {
                try {
                    auto type_result = infer_type(*arg);
                    if (type_result) {
                        signature.push_back(std::move(type_result.value()));
                    } else {
                        signature.push_back(type_utils::make_atom_type()); // Default
                    }
                } catch (...) {
                    signature.push_back(type_utils::make_atom_type()); // Default
                }
            }
            
            if (!global_env.has_predicate_signature(fact->head->functor)) {
                global_env.set_predicate_signature(fact->head->functor, std::move(signature));
            }
        }
    }
    
    // Third pass: type check all clauses
    auto env = global_env.create_child();
    
    for (const auto& clause : program.clauses) {
        if (auto fact = dynamic_cast<const ast::Fact*>(clause.get())) {
            check_fact(*fact, *env);
        } else if (auto rule = dynamic_cast<const ast::Rule*>(clause.get())) {
            check_rule(*rule, *env);
        } else if (auto query = dynamic_cast<const ast::Query*>(clause.get())) {
            check_query(*query, *env);
        }
    }
}

std::expected<std::unique_ptr<ast::Type>, TypeError> 
TypeChecker::infer_type(const ast::Term& term) {
    try {
        auto env = global_env.create_child();
        return infer_term_type(term, *env);
    } catch (const TypeError& e) {
        return std::unexpected(e);
    }
}

std::unique_ptr<ast::Type> TypeChecker::infer_term_type(const ast::Term& term, TypeEnvironment& env) {
    if (auto atom = dynamic_cast<const ast::Atom*>(&term)) {
        return infer_atom_type(*atom);
    } else if (auto variable = dynamic_cast<const ast::Variable*>(&term)) {
        return infer_variable_type(*variable, env);
    } else if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        return infer_compound_type(*compound, env);
    } else if (auto number = dynamic_cast<const ast::Number*>(&term)) {
        return infer_number_type(*number);
    } else if (auto string = dynamic_cast<const ast::String*>(&term)) {
        return infer_string_type(*string);
    }
    
    throw TypeError("Unknown term type", term.position);
}

std::unique_ptr<ast::Type> TypeChecker::infer_atom_type(const ast::Atom& atom) {
    return type_utils::make_atom_type();
}

std::unique_ptr<ast::Type> TypeChecker::infer_variable_type(const ast::Variable& var, TypeEnvironment& env) {
    if (env.has_variable_type(var.name)) {
        // Return copy of existing type
        return type_utils::make_atom_type(); // Simplified
    }
    
    // Create fresh type variable
    auto fresh_type = type_utils::make_compound_type("_T" + var.name);
    env.set_variable_type(var.name, type_utils::make_compound_type("_T" + var.name));
    
    return fresh_type;
}

std::unique_ptr<ast::Type> TypeChecker::infer_compound_type(const ast::Compound& compound, TypeEnvironment& env) {
    // Check if we have a signature for this predicate
    if (const auto* signature = env.get_predicate_signature(compound.functor)) {
        if (signature->size() != compound.arguments.size()) {
            throw TypeError(
                std::format("Arity mismatch for predicate {}: expected {}, got {}", 
                          compound.functor, signature->size(), compound.arguments.size()),
                compound.position);
        }
        
        // Type check arguments against signature
        for (size_t i = 0; i < compound.arguments.size(); ++i) {
            auto arg_type = infer_term_type(*compound.arguments[i], env);
            
            if (!types_compatible(*arg_type, *(*signature)[i])) {
                throw TypeError(
                    std::format("Type mismatch in argument {} of predicate {}", 
                              i + 1, compound.functor),
                    compound.arguments[i]->position);
            }
        }
        
        return type_utils::make_atom_type(); // Predicates have no return type in logic programming
    }
    
    // No signature found - infer from arguments
    return type_utils::make_atom_type();
}

std::unique_ptr<ast::Type> TypeChecker::infer_number_type(const ast::Number& number) {
    return type_utils::make_integer_type();
}

std::unique_ptr<ast::Type> TypeChecker::infer_string_type(const ast::String& string) {
    return type_utils::make_compound_type("string");
}

bool TypeChecker::types_compatible(const ast::Type& type1, const ast::Type& type2) const {
    // Simplified type compatibility check
    if (auto atom1 = dynamic_cast<const ast::AtomType*>(&type1)) {
        return dynamic_cast<const ast::AtomType*>(&type2) != nullptr;
    }
    
    if (auto int1 = dynamic_cast<const ast::IntegerType*>(&type1)) {
        return dynamic_cast<const ast::IntegerType*>(&type2) != nullptr;
    }
    
    if (auto comp1 = dynamic_cast<const ast::CompoundType*>(&type1)) {
        if (auto comp2 = dynamic_cast<const ast::CompoundType*>(&type2)) {
            return comp1->name == comp2->name && 
                   comp1->parameters.size() == comp2->parameters.size();
        }
    }
    
    return false;
}

void TypeChecker::check_fact(const ast::Fact& fact, TypeEnvironment& env) {
    // Type check the fact head
    infer_compound_type(*fact.head, env);
}

void TypeChecker::check_rule(const ast::Rule& rule, TypeEnvironment& env) {
    // Type check rule head
    infer_compound_type(*rule.head, env);
    
    // Type check rule body
    for (const auto& body_expr : rule.body) {
        check_expression(*body_expr, env);
    }
    
    // Type check conditions
    for (const auto& condition : rule.conditions) {
        check_expression(*condition, env);
    }
}

void TypeChecker::check_query(const ast::Query& query, TypeEnvironment& env) {
    // Type check query goals
    for (const auto& goal : query.goals) {
        check_expression(*goal, env);
    }
    
    // Type check conditions
    for (const auto& condition : query.conditions) {
        check_expression(*condition, env);
    }
}

void TypeChecker::check_expression(const ast::Expression& expr, TypeEnvironment& env) {
    if (auto call_expr = dynamic_cast<const ast::CallExpr*>(&expr)) {
        infer_compound_type(*call_expr->compound, env);
    }
    // Other expression types would be handled here...
}

// Pattern implementations
bool LiteralPattern::matches(const ast::Term& term, Substitution& bindings) const {
    UnificationEngine engine;
    auto result = engine.unify(*literal, term);
    
    if (result.success) {
        bindings = bindings.compose(result.substitution);
        return true;
    }
    return false;
}

std::unique_ptr<MatchPattern> LiteralPattern::clone() const {
    return std::make_unique<LiteralPattern>(term_utils::deep_copy(*literal));
}

bool VariablePattern::matches(const ast::Term& term, Substitution& bindings) const {
    // Variables always match, bind the variable to the term
    bindings.bind(variable_name, term_utils::deep_copy(term));
    return true;
}

std::unique_ptr<MatchPattern> VariablePattern::clone() const {
    return std::make_unique<VariablePattern>(variable_name);
}

bool CompoundPattern::matches(const ast::Term& term, Substitution& bindings) const {
    if (auto compound = dynamic_cast<const ast::Compound*>(&term)) {
        if (compound->functor != functor || 
            compound->arguments.size() != argument_patterns.size()) {
            return false;
        }
        
        // Match all argument patterns
        for (size_t i = 0; i < argument_patterns.size(); ++i) {
            if (!argument_patterns[i]->matches(*compound->arguments[i], bindings)) {
                return false;
            }
        }
        
        return true;
    }
    return false;
}

std::unique_ptr<MatchPattern> CompoundPattern::clone() const {
    auto cloned = std::make_unique<CompoundPattern>(functor);
    
    for (const auto& pattern : argument_patterns) {
        cloned->argument_patterns.push_back(pattern->clone());
    }
    
    return std::move(cloned);
}

bool WildcardPattern::matches(const ast::Term& term, Substitution& bindings) const {
    return true; // Wildcard matches everything
}

std::unique_ptr<MatchPattern> WildcardPattern::clone() const {
    return std::make_unique<WildcardPattern>();
}

// PatternMatcher implementation
PatternMatcher::MatchResult PatternMatcher::match(const ast::Term& term, const MatchPattern& pattern) const {
    Substitution bindings;
    
    if (pattern.matches(term, bindings)) {
        return MatchResult::success(std::move(bindings));
    }
    
    return MatchResult::failure();
}

PatternMatcher::MatchResult PatternMatcher::match_all(
    const std::vector<std::reference_wrapper<const ast::Term>>& terms,
    const std::vector<std::reference_wrapper<const MatchPattern>>& patterns) const {
    
    if (terms.size() != patterns.size()) {
        return MatchResult::failure();
    }
    
    Substitution combined_bindings;
    
    for (size_t i = 0; i < terms.size(); ++i) {
        auto result = match(terms[i].get(), patterns[i].get());
        if (!result.success) {
            return MatchResult::failure();
        }
        
        combined_bindings = combined_bindings.compose(result.bindings);
    }
    
    return MatchResult::success(std::move(combined_bindings));
}

// Type utilities
namespace type_utils {

std::string type_to_string(const ast::Type& type) {
    if (dynamic_cast<const ast::AtomType*>(&type)) {
        return "atom";
    } else if (dynamic_cast<const ast::IntegerType*>(&type)) {
        return "i32";
    } else if (auto compound_type = dynamic_cast<const ast::CompoundType*>(&type)) {
        if (compound_type->parameters.empty()) {
            return compound_type->name;
        } else {
            std::string result = compound_type->name + "<";
            for (size_t i = 0; i < compound_type->parameters.size(); ++i) {
                if (i > 0) result += ", ";
                result += type_to_string(*compound_type->parameters[i]);
            }
            result += ">";
            return result;
        }
    }
    
    return "unknown";
}

bool is_ground_type(const ast::Type& type) {
    if (auto compound_type = dynamic_cast<const ast::CompoundType*>(&type)) {
        // Type variables start with underscore
        if (compound_type->name.starts_with("_T")) {
            return false;
        }
        
        // Check if all parameters are ground
        for (const auto& param : compound_type->parameters) {
            if (!is_ground_type(*param)) {
                return false;
            }
        }
    }
    
    return true;
}

size_t get_type_arity(const ast::Type& type) {
    if (auto compound_type = dynamic_cast<const ast::CompoundType*>(&type)) {
        return compound_type->parameters.size();
    }
    return 0;
}

std::unique_ptr<ast::Type> make_atom_type() {
    return std::make_unique<ast::AtomType>(Position{});
}

std::unique_ptr<ast::Type> make_integer_type() {
    return std::make_unique<ast::IntegerType>(Position{});
}

std::unique_ptr<ast::Type> make_compound_type(const std::string& name) {
    return std::make_unique<ast::CompoundType>(Position{}, name);
}

} // namespace type_utils

} // namespace logicpp