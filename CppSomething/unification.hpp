#pragma once

#include "ast.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <functional>

namespace logicpp {

// Substitution maps variable names to terms
class Substitution {
private:
    std::unordered_map<std::string, std::unique_ptr<ast::Term>> bindings;

public:
    Substitution() = default;
    Substitution(const Substitution& other);
    Substitution& operator=(const Substitution& other);
    Substitution(Substitution&&) = default;
    Substitution& operator=(Substitution&&) = default;

    // Add a binding from variable to term
    void bind(const std::string& variable, std::unique_ptr<ast::Term> term);
    
    // Get the binding for a variable
    const ast::Term* lookup(const std::string& variable) const;
    
    // Check if a variable is bound
    bool is_bound(const std::string& variable) const;
    
    // Apply substitution to a term
    std::unique_ptr<ast::Term> apply(const ast::Term& term) const;
    
    // Compose two substitutions
    Substitution compose(const Substitution& other) const;
    
    // Get all variable names
    std::vector<std::string> variables() const;
    
    // Check if substitution is empty
    bool empty() const { return bindings.empty(); }
    
    // Clear all bindings
    void clear() { bindings.clear(); }
    
    // Debug: print substitution
    std::string to_string() const;
};

// Unification result
struct UnificationResult {
    bool success;
    Substitution substitution;
    
    UnificationResult(bool s) : success(s) {}
    UnificationResult(bool s, Substitution sub) 
        : success(s), substitution(std::move(sub)) {}
    
    static UnificationResult failure() { return UnificationResult(false); }
    static UnificationResult success(Substitution sub = {}) { 
        return UnificationResult(true, std::move(sub)); 
    }
};

// Main unification engine
class UnificationEngine {
private:
    // Helper methods for different term types
    UnificationResult unify_variables(
        const ast::Variable& var1, 
        const ast::Variable& var2,
        Substitution& substitution) const;
    
    UnificationResult unify_variable_with_term(
        const ast::Variable& variable,
        const ast::Term& term,
        Substitution& substitution) const;
    
    UnificationResult unify_compounds(
        const ast::Compound& comp1,
        const ast::Compound& comp2,
        Substitution& substitution) const;
    
    UnificationResult unify_atoms(
        const ast::Atom& atom1,
        const ast::Atom& atom2) const;
    
    UnificationResult unify_numbers(
        const ast::Number& num1,
        const ast::Number& num2) const;
    
    UnificationResult unify_strings(
        const ast::String& str1,
        const ast::String& str2) const;
    
    // Occur check: prevents infinite structures
    bool occurs_check(const std::string& variable, const ast::Term& term) const;
    
    // Deep clone a term
    std::unique_ptr<ast::Term> clone_term(const ast::Term& term) const;

public:
    // Main unification method
    UnificationResult unify(const ast::Term& term1, const ast::Term& term2) const;
    
    // Unify with existing substitution
    UnificationResult unify(
        const ast::Term& term1, 
        const ast::Term& term2,
        const Substitution& existing_substitution) const;
    
    // Unify lists of terms (for compound arguments)
    UnificationResult unify_lists(
        const std::vector<std::unique_ptr<ast::Term>>& terms1,
        const std::vector<std::unique_ptr<ast::Term>>& terms2,
        Substitution& substitution) const;
    
    // Check if two terms are unifiable without computing substitution
    bool unifiable(const ast::Term& term1, const ast::Term& term2) const;
    
    // Rename variables in a term to avoid conflicts
    std::unique_ptr<ast::Term> rename_variables(
        const ast::Term& term, 
        const std::string& prefix = "V") const;
    
    // Generate fresh variable name
    std::string fresh_variable_name(const std::string& base = "X") const;
};

// Utility functions for term manipulation
namespace term_utils {
    
    // Check if term is a variable
    bool is_variable(const ast::Term& term);
    
    // Check if term is ground (contains no variables)
    bool is_ground(const ast::Term& term);
    
    // Get all variables in a term
    std::vector<std::string> get_variables(const ast::Term& term);
    
    // Compare terms for structural equality
    bool structurally_equal(const ast::Term& term1, const ast::Term& term2);
    
    // Convert term to string representation
    std::string term_to_string(const ast::Term& term);
    
    // Deep copy a term
    std::unique_ptr<ast::Term> deep_copy(const ast::Term& term);
    
} // namespace term_utils

} // namespace logicpp