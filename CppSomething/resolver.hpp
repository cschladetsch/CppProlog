#pragma once

#include "ast.hpp"
#include "unification.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <set>
#include <queue>

namespace logicpp {

// Knowledge base containing facts and rules
class KnowledgeBase {
private:
    std::vector<std::unique_ptr<ast::Fact>> facts;
    std::vector<std::unique_ptr<ast::Rule>> rules;
    
public:
    void add_fact(std::unique_ptr<ast::Fact> fact);
    void add_rule(std::unique_ptr<ast::Rule> rule);
    
    // Get all clauses (facts and rules) that might unify with a goal
    std::vector<const ast::Clause*> get_matching_clauses(const ast::Compound& goal) const;
    
    // Clear all knowledge
    void clear();
    
    // Statistics
    size_t fact_count() const { return facts.size(); }
    size_t rule_count() const { return rules.size(); }
    
    // Debug: print all knowledge
    void print_knowledge() const;
};

// A goal in the resolution process
struct Goal {
    std::unique_ptr<ast::Compound> compound;
    Substitution substitution;
    size_t depth;
    
    Goal(std::unique_ptr<ast::Compound> comp, Substitution sub = {}, size_t d = 0)
        : compound(std::move(comp)), substitution(std::move(sub)), depth(d) {}
};

// Solution to a query
struct Solution {
    Substitution bindings;
    std::vector<std::string> variable_names;  // Original query variables
    
    // Check if solution binds specific variable
    bool binds(const std::string& variable) const;
    
    // Get binding for variable as string
    std::string get_binding_string(const std::string& variable) const;
    
    // Convert to human-readable format
    std::string to_string() const;
};

// Query resolution options
struct ResolverOptions {
    size_t max_depth = 1000;           // Maximum recursion depth
    size_t max_solutions = 100;        // Maximum number of solutions to find
    bool find_all_solutions = false;   // Find all solutions vs first solution
    bool trace_execution = false;      // Enable execution tracing
    
    static ResolverOptions default_options() { return {}; }
    static ResolverOptions find_all() { 
        ResolverOptions opts;
        opts.find_all_solutions = true;
        return opts;
    }
};

// Query resolver using SLD resolution
class QueryResolver {
private:
    const KnowledgeBase& knowledge_base;
    UnificationEngine unification_engine;
    ResolverOptions options;
    
    // Trace execution if enabled
    void trace(const std::string& message, size_t depth = 0) const;
    
    // Resolve a single goal
    void resolve_goal(
        const Goal& goal,
        std::vector<Goal>& remaining_goals,
        std::vector<Solution>& solutions) const;
    
    // Apply substitution to remaining goals
    std::vector<Goal> apply_substitution_to_goals(
        const std::vector<Goal>& goals,
        const Substitution& substitution) const;
    
    // Check if depth limit exceeded
    bool depth_limit_exceeded(size_t depth) const;
    
    // Rename variables in clause to avoid conflicts
    std::unique_ptr<ast::Clause> rename_clause_variables(
        const ast::Clause& clause,
        const std::string& prefix) const;
    
    // Generate unique variable prefix
    std::string generate_variable_prefix() const;

public:
    QueryResolver(const KnowledgeBase& kb, ResolverOptions opts = ResolverOptions::default_options())
        : knowledge_base(kb), options(opts) {}
    
    // Resolve a query and return all solutions
    std::vector<Solution> resolve_query(const ast::Query& query) const;
    
    // Resolve a single compound goal
    std::vector<Solution> resolve_goal(const ast::Compound& goal) const;
    
    // Check if a goal can be proven (returns true/false)
    bool can_prove(const ast::Compound& goal) const;
    
    // Get first solution if any
    std::optional<Solution> get_first_solution(const ast::Compound& goal) const;
    
    // Set resolver options
    void set_options(const ResolverOptions& new_options) { options = new_options; }
    const ResolverOptions& get_options() const { return options; }
};

// Interactive query engine
class QueryEngine {
private:
    KnowledgeBase knowledge_base;
    QueryResolver resolver;
    
public:
    QueryEngine() : resolver(knowledge_base) {}
    
    // Load program into knowledge base
    void load_program(const ast::Program& program);
    
    // Add individual clauses
    void add_fact(std::unique_ptr<ast::Fact> fact);
    void add_rule(std::unique_ptr<ast::Rule> rule);
    
    // Execute query
    std::vector<Solution> query(const std::string& query_string);
    std::vector<Solution> query(const ast::Query& query);
    
    // Quick yes/no queries
    bool ask(const std::string& goal_string);
    bool ask(const ast::Compound& goal);
    
    // Clear knowledge base
    void clear() { knowledge_base.clear(); }
    
    // Get statistics
    struct Stats {
        size_t facts;
        size_t rules;
        size_t total_clauses() const { return facts + rules; }
    };
    Stats get_stats() const;
    
    // Set resolver options
    void set_resolver_options(const ResolverOptions& options) {
        resolver.set_options(options);
    }
};

// Utility functions for working with solutions
namespace solution_utils {
    
    // Print solutions in a readable format
    void print_solutions(const std::vector<Solution>& solutions);
    
    // Check if solutions contain a specific binding
    bool solutions_contain_binding(
        const std::vector<Solution>& solutions,
        const std::string& variable,
        const std::string& value);
    
    // Filter solutions by variable binding
    std::vector<Solution> filter_solutions_by_binding(
        const std::vector<Solution>& solutions,
        const std::string& variable,
        const std::function<bool(const std::string&)>& predicate);
    
} // namespace solution_utils

} // namespace logicpp