#pragma once

#include "ast.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <expected>

namespace logicpp {

// Type checking errors
class TypeError : public std::exception {
private:
    std::string message;
    Position position;

public:
    TypeError(std::string msg, Position pos) 
        : message(std::move(msg)), position(pos) {}
    
    const char* what() const noexcept override { return message.c_str(); }
    Position where() const noexcept { return position; }
};

// Type environment for variable and predicate types
class TypeEnvironment {
private:
    std::unordered_map<std::string, std::unique_ptr<ast::Type>> variable_types;
    std::unordered_map<std::string, std::vector<std::unique_ptr<ast::Type>>> predicate_types;
    
public:
    // Variable types
    void set_variable_type(const std::string& var, std::unique_ptr<ast::Type> type);
    const ast::Type* get_variable_type(const std::string& var) const;
    bool has_variable_type(const std::string& var) const;
    
    // Predicate types (signature)
    void set_predicate_signature(const std::string& predicate, 
                                std::vector<std::unique_ptr<ast::Type>> signature);
    const std::vector<std::unique_ptr<ast::Type>>* get_predicate_signature(const std::string& predicate) const;
    bool has_predicate_signature(const std::string& predicate) const;
    
    // Create child environment (for scoping)
    std::unique_ptr<TypeEnvironment> create_child() const;
    
    // Merge with another environment
    void merge(const TypeEnvironment& other);
};

// Type inference and checking
class TypeChecker {
private:
    TypeEnvironment global_env;
    
    // Helper methods
    std::unique_ptr<ast::Type> infer_term_type(const ast::Term& term, TypeEnvironment& env);
    std::unique_ptr<ast::Type> infer_atom_type(const ast::Atom& atom);
    std::unique_ptr<ast::Type> infer_variable_type(const ast::Variable& var, TypeEnvironment& env);
    std::unique_ptr<ast::Type> infer_compound_type(const ast::Compound& compound, TypeEnvironment& env);
    std::unique_ptr<ast::Type> infer_number_type(const ast::Number& number);
    std::unique_ptr<ast::Type> infer_string_type(const ast::String& string);
    
    // Type compatibility
    bool types_compatible(const ast::Type& type1, const ast::Type& type2) const;
    std::unique_ptr<ast::Type> unify_types(const ast::Type& type1, const ast::Type& type2);
    
    // Type checking for clauses
    void check_fact(const ast::Fact& fact, TypeEnvironment& env);
    void check_rule(const ast::Rule& rule, TypeEnvironment& env);
    void check_query(const ast::Query& query, TypeEnvironment& env);
    
    // Expression type checking
    void check_expression(const ast::Expression& expr, TypeEnvironment& env);
    
public:
    TypeChecker();
    
    // Main type checking methods
    void check_program(const ast::Program& program);
    void add_builtin_types();
    
    // Type inference
    std::expected<std::unique_ptr<ast::Type>, TypeError> infer_type(const ast::Term& term);
    
    // Manual type annotations
    void add_predicate_type(const std::string& predicate, 
                           std::vector<std::unique_ptr<ast::Type>> signature);
    
    // Get type environment
    const TypeEnvironment& get_global_environment() const { return global_env; }
};

// Pattern matching implementation
struct MatchPattern {
    virtual ~MatchPattern() = default;
    virtual bool matches(const ast::Term& term, Substitution& bindings) const = 0;
    virtual std::unique_ptr<MatchPattern> clone() const = 0;
};

struct LiteralPattern : MatchPattern {
    std::unique_ptr<ast::Term> literal;
    
    LiteralPattern(std::unique_ptr<ast::Term> lit) : literal(std::move(lit)) {}
    
    bool matches(const ast::Term& term, Substitution& bindings) const override;
    std::unique_ptr<MatchPattern> clone() const override;
};

struct VariablePattern : MatchPattern {
    std::string variable_name;
    
    VariablePattern(std::string name) : variable_name(std::move(name)) {}
    
    bool matches(const ast::Term& term, Substitution& bindings) const override;
    std::unique_ptr<MatchPattern> clone() const override;
};

struct CompoundPattern : MatchPattern {
    std::string functor;
    std::vector<std::unique_ptr<MatchPattern>> argument_patterns;
    
    CompoundPattern(std::string f) : functor(std::move(f)) {}
    
    bool matches(const ast::Term& term, Substitution& bindings) const override;
    std::unique_ptr<MatchPattern> clone() const override;
};

struct WildcardPattern : MatchPattern {
    bool matches(const ast::Term& term, Substitution& bindings) const override;
    std::unique_ptr<MatchPattern> clone() const override;
};

// Pattern matching engine
class PatternMatcher {
public:
    struct MatchResult {
        bool success;
        Substitution bindings;
        
        MatchResult(bool s) : success(s) {}
        MatchResult(bool s, Substitution b) : success(s), bindings(std::move(b)) {}
        
        static MatchResult success(Substitution bindings = {}) {
            return MatchResult(true, std::move(bindings));
        }
        
        static MatchResult failure() {
            return MatchResult(false);
        }
    };
    
    // Match a term against a pattern
    MatchResult match(const ast::Term& term, const MatchPattern& pattern) const;
    
    // Match multiple terms against patterns
    MatchResult match_all(
        const std::vector<std::reference_wrapper<const ast::Term>>& terms,
        const std::vector<std::reference_wrapper<const MatchPattern>>& patterns) const;
};

// Enhanced resolver with pattern matching and type checking
class TypedResolver {
private:
    TypeChecker type_checker;
    PatternMatcher pattern_matcher;
    
public:
    TypedResolver();
    
    // Type-safe query resolution
    // (extends basic resolver with type checking)
    
    // Pattern-based goal matching
    // (uses pattern matching for more sophisticated goal resolution)
};

// Type utilities
namespace type_utils {
    
    // Convert type to string representation
    std::string type_to_string(const ast::Type& type);
    
    // Check if type is ground (fully specified)
    bool is_ground_type(const ast::Type& type);
    
    // Get type arity (for compound types)
    size_t get_type_arity(const ast::Type& type);
    
    // Create basic types
    std::unique_ptr<ast::Type> make_atom_type();
    std::unique_ptr<ast::Type> make_integer_type();
    std::unique_ptr<ast::Type> make_compound_type(const std::string& name);
    
} // namespace type_utils

} // namespace logicpp