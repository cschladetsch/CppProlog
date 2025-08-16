#include "builtin_predicates.h"
#include "unification.h"
#include <cmath>

namespace prolog {

std::unordered_map<std::string, BuiltinPredicates::BuiltinHandler> BuiltinPredicates::builtins_;

void BuiltinPredicates::registerBuiltins() {
    if (!builtins_.empty()) return; // Already registered
    
    // Arithmetic
    builtins_[makeKey("is", 2)] = is;
    builtins_[makeKey("+", 3)] = add;
    builtins_[makeKey("-", 3)] = subtract;
    builtins_[makeKey("*", 3)] = multiply;
    builtins_[makeKey("/", 3)] = divide;
    
    // Comparison
    builtins_[makeKey("=", 2)] = equal;
    builtins_[makeKey("\\=", 2)] = notEqual;
    builtins_[makeKey("<", 2)] = lessThan;
    builtins_[makeKey(">", 2)] = greaterThan;
    
    // List operations
    builtins_[makeKey("append", 3)] = append;
    builtins_[makeKey("member", 2)] = member;
    builtins_[makeKey("length", 2)] = length;
    
    // Type checking
    builtins_[makeKey("var", 1)] = var;
    builtins_[makeKey("nonvar", 1)] = nonvar;
    builtins_[makeKey("atom", 1)] = atom;
    builtins_[makeKey("number", 1)] = number;
    
    // Control
    builtins_[makeKey("!", 0)] = cut;
    builtins_[makeKey("fail", 0)] = fail;
    builtins_[makeKey("true", 0)] = true_pred;
}

bool BuiltinPredicates::isBuiltin(const std::string& functor, size_t arity) {
    return builtins_.find(makeKey(functor, arity)) != builtins_.end();
}

bool BuiltinPredicates::callBuiltin(const std::string& functor, size_t arity, 
                                   const TermList& args, Substitution& bindings,
                                   std::function<bool(const Solution&)> callback) {
    auto it = builtins_.find(makeKey(functor, arity));
    if (it != builtins_.end()) {
        return it->second(args, bindings, callback);
    }
    return false;
}

std::string BuiltinPredicates::makeKey(const std::string& functor, size_t arity) {
    return functor + "/" + std::to_string(arity);
}

bool BuiltinPredicates::is(const TermList& args, Substitution& bindings, 
                          std::function<bool(const Solution&)> callback) {
    if (args.size() != 2) return false;
    
    TermPtr left = Unification::applySubstitution(args[0], bindings);
    TermPtr right = Unification::applySubstitution(args[1], bindings);
    
    if (!isNumber(right)) return false;
    
    double value = getNumericValue(right);
    
    if (unifyWithNumber(left, value, bindings)) {
        Solution solution{bindings};
        return callback(solution);
    }
    
    return false;
}

bool BuiltinPredicates::add(const TermList& args, Substitution& bindings, 
                           std::function<bool(const Solution&)> callback) {
    if (args.size() != 3) return false;
    
    TermPtr arg1 = Unification::applySubstitution(args[0], bindings);
    TermPtr arg2 = Unification::applySubstitution(args[1], bindings);
    TermPtr result = Unification::applySubstitution(args[2], bindings);
    
    if (!isNumber(arg1) || !isNumber(arg2)) return false;
    
    double val1 = getNumericValue(arg1);
    double val2 = getNumericValue(arg2);
    double sum = val1 + val2;
    
    if (unifyWithNumber(result, sum, bindings)) {
        Solution solution{bindings};
        return callback(solution);
    }
    
    return false;
}

bool BuiltinPredicates::equal(const TermList& args, Substitution& bindings, 
                             std::function<bool(const Solution&)> callback) {
    if (args.size() != 2) return false;
    
    TermPtr left = Unification::applySubstitution(args[0], bindings);
    TermPtr right = Unification::applySubstitution(args[1], bindings);
    
    auto unification = Unification::unify(left, right, bindings);
    if (unification) {
        Solution solution{*unification};
        return callback(solution);
    }
    
    return false;
}

bool BuiltinPredicates::append(const TermList& args, Substitution& bindings, 
                              std::function<bool(const Solution&)> callback) {
    if (args.size() != 3) return false;
    
    TermPtr list1 = Unification::applySubstitution(args[0], bindings);
    TermPtr list2 = Unification::applySubstitution(args[1], bindings);
    TermPtr result = Unification::applySubstitution(args[2], bindings);
    
    // Simple implementation for ground terms
    if (list1->is<List>() && list2->is<List>()) {
        auto l1 = list1->as<List>();
        auto l2 = list2->as<List>();
        
        TermList combined = l1->elements();
        const auto& l2_elements = l2->elements();
        combined.insert(combined.end(), l2_elements.begin(), l2_elements.end());
        
        TermPtr combined_list = makeList(std::move(combined));
        
        auto unification = Unification::unify(result, combined_list, bindings);
        if (unification) {
            Solution solution{*unification};
            return callback(solution);
        }
    }
    
    return false;
}

bool BuiltinPredicates::member(const TermList& args, Substitution& bindings, 
                              std::function<bool(const Solution&)> callback) {
    if (args.size() != 2) return false;
    
    TermPtr element = Unification::applySubstitution(args[0], bindings);
    TermPtr list = Unification::applySubstitution(args[1], bindings);
    
    if (list->is<List>()) {
        auto l = list->as<List>();
        for (const auto& elem : l->elements()) {
            auto unification = Unification::unify(element, elem, bindings);
            if (unification) {
                Solution solution{*unification};
                if (!callback(solution)) {
                    return false; // Callback requested stop
                }
            }
        }
        return true; // All possibilities explored
    }
    
    return false;
}

bool BuiltinPredicates::var(const TermList& args, Substitution& bindings, 
                           std::function<bool(const Solution&)> callback) {
    if (args.size() != 1) return false;
    
    TermPtr term = Unification::applySubstitution(args[0], bindings);
    
    if (term->type() == TermType::VARIABLE) {
        Solution solution{bindings};
        return callback(solution);
    }
    
    return false;
}

bool BuiltinPredicates::true_pred(const TermList& args, Substitution& bindings, 
                                 std::function<bool(const Solution&)> callback) {
    Solution solution{bindings};
    return callback(solution);
}

bool BuiltinPredicates::fail(const TermList& args, Substitution& bindings, 
                            std::function<bool(const Solution&)> callback) {
    return false;
}

// Utility functions
bool BuiltinPredicates::isNumber(const TermPtr& term) {
    return term->type() == TermType::INTEGER || term->type() == TermType::FLOAT;
}

double BuiltinPredicates::getNumericValue(const TermPtr& term) {
    if (term->type() == TermType::INTEGER) {
        return static_cast<double>(term->as<Integer>()->value());
    } else if (term->type() == TermType::FLOAT) {
        return term->as<Float>()->value();
    }
    return 0.0;
}

bool BuiltinPredicates::unifyWithNumber(const TermPtr& term, double value, Substitution& bindings) {
    TermPtr number_term;
    if (std::floor(value) == value && value >= INT64_MIN && value <= INT64_MAX) {
        number_term = makeInteger(static_cast<int64_t>(value));
    } else {
        number_term = makeFloat(value);
    }
    
    auto unification = Unification::unify(term, number_term, bindings);
    return unification.has_value();
}

// Stub implementations for remaining predicates
bool BuiltinPredicates::subtract(const TermList& args, Substitution& bindings, 
                                std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::multiply(const TermList& args, Substitution& bindings, 
                                std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::divide(const TermList& args, Substitution& bindings, 
                              std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::notEqual(const TermList& args, Substitution& bindings, 
                                std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::lessThan(const TermList& args, Substitution& bindings, 
                                std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::greaterThan(const TermList& args, Substitution& bindings, 
                                   std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::length(const TermList& args, Substitution& bindings, 
                              std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::nonvar(const TermList& args, Substitution& bindings, 
                              std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::atom(const TermList& args, Substitution& bindings, 
                            std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::number(const TermList& args, Substitution& bindings, 
                              std::function<bool(const Solution&)> callback) { return false; }
bool BuiltinPredicates::cut(const TermList& args, Substitution& bindings, 
                           std::function<bool(const Solution&)> callback) { return false; }

}