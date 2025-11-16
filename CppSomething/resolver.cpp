#include "resolver.hpp"
#include "parser.hpp"
#include <format>
#include <iostream>
#include <algorithm>
#include <atomic>

namespace logicpp {

// KnowledgeBase implementation
void KnowledgeBase::add_fact(std::unique_ptr<ast::Fact> fact) {
    facts.push_back(std::move(fact));
}

void KnowledgeBase::add_rule(std::unique_ptr<ast::Rule> rule) {
    rules.push_back(std::move(rule));
}

std::vector<const ast::Clause*> KnowledgeBase::get_matching_clauses(const ast::Compound& goal) const {
    std::vector<const ast::Clause*> matching;
    
    // Check facts
    for (const auto& fact : facts) {
        if (fact->head->functor == goal.functor && 
            fact->head->arguments.size() == goal.arguments.size()) {
            matching.push_back(fact.get());
        }
    }
    
    // Check rules
    for (const auto& rule : rules) {
        if (rule->head->functor == goal.functor && 
            rule->head->arguments.size() == goal.arguments.size()) {
            matching.push_back(rule.get());
        }
    }
    
    return matching;
}

void KnowledgeBase::clear() {
    facts.clear();
    rules.clear();
}

void KnowledgeBase::print_knowledge() const {
    std::cout << "Facts:\n";
    for (const auto& fact : facts) {
        std::cout << "  " << term_utils::term_to_string(*fact->head) << ".\n";
    }
    
    std::cout << "Rules:\n";
    for (const auto& rule : rules) {
        std::cout << "  " << term_utils::term_to_string(*rule->head) << " :- ";
        for (size_t i = 0; i < rule->body.size(); ++i) {
            if (i > 0) std::cout << ", ";
            // Simplified - would need proper expression printing
            std::cout << "<?>";
        }
        std::cout << ".\n";
    }
}

// Solution implementation
bool Solution::binds(const std::string& variable) const {
    return bindings.is_bound(variable);
}

std::string Solution::get_binding_string(const std::string& variable) const {
    if (const ast::Term* term = bindings.lookup(variable)) {
        return term_utils::term_to_string(*term);
    }
    return variable;  // Unbound variable
}

std::string Solution::to_string() const {
    if (bindings.empty()) {
        return "true";
    }
    
    std::string result = "{ ";
    bool first = true;
    
    for (const std::string& var : variable_names) {
        if (bindings.is_bound(var)) {
            if (!first) result += ", ";
            result += var + " = " + get_binding_string(var);
            first = false;
        }
    }
    
    result += " }";
    return result;
}

// QueryResolver implementation
void QueryResolver::trace(const std::string& message, size_t depth) const {
    if (options.trace_execution) {
        for (size_t i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        std::cout << message << std::endl;
    }
}

std::vector<Solution> QueryResolver::resolve_query(const ast::Query& query) const {
    std::vector<Solution> solutions;
    
    if (query.goals.empty()) {
        return solutions;
    }
    
    // Extract variable names from query
    std::set<std::string> query_vars;
    for (const auto& goal : query.goals) {
        // Simplified - would need proper variable extraction from expressions
        if (auto call_expr = dynamic_cast<const ast::CallExpr*>(goal.get())) {
            auto vars = term_utils::get_variables(*call_expr->compound);
            query_vars.insert(vars.begin(), vars.end());
        }
    }
    
    std::vector<std::string> var_names(query_vars.begin(), query_vars.end());
    
    // Convert first goal to compound and start resolution
    if (auto call_expr = dynamic_cast<const ast::CallExpr*>(query.goals[0].get())) {
        auto compound_copy = std::unique_ptr<ast::Compound>(
            dynamic_cast<ast::Compound*>(term_utils::deep_copy(*call_expr->compound).release()));
        
        Goal initial_goal(std::move(compound_copy));
        
        std::vector<Goal> remaining_goals;
        
        // Add remaining goals from query
        for (size_t i = 1; i < query.goals.size(); ++i) {
            if (auto call_expr_i = dynamic_cast<const ast::CallExpr*>(query.goals[i].get())) {
                auto compound_i = std::unique_ptr<ast::Compound>(
                    dynamic_cast<ast::Compound*>(term_utils::deep_copy(*call_expr_i->compound).release()));
                remaining_goals.emplace_back(std::move(compound_i));
            }
        }
        
        resolve_goal(initial_goal, remaining_goals, solutions);
        
        // Set variable names in solutions
        for (auto& solution : solutions) {
            solution.variable_names = var_names;
        }
    }
    
    return solutions;
}

std::vector<Solution> QueryResolver::resolve_goal(const ast::Compound& goal) const {
    auto compound_copy = std::unique_ptr<ast::Compound>(
        dynamic_cast<ast::Compound*>(term_utils::deep_copy(goal).release()));
    
    Goal goal_obj(std::move(compound_copy));
    std::vector<Goal> remaining_goals;
    std::vector<Solution> solutions;
    
    resolve_goal(goal_obj, remaining_goals, solutions);
    
    // Extract variable names from goal
    auto var_names = term_utils::get_variables(goal);
    for (auto& solution : solutions) {
        solution.variable_names = var_names;
    }
    
    return solutions;
}

void QueryResolver::resolve_goal(
    const Goal& goal,
    std::vector<Goal>& remaining_goals,
    std::vector<Solution>& solutions) const {
    
    if (depth_limit_exceeded(goal.depth)) {
        trace(std::format("Depth limit exceeded for goal: {}", 
              term_utils::term_to_string(*goal.compound)), goal.depth);
        return;
    }
    
    trace(std::format("Resolving goal: {} at depth {}", 
          term_utils::term_to_string(*goal.compound), goal.depth), goal.depth);
    
    // Apply current substitution to the goal
    auto applied_goal = std::unique_ptr<ast::Compound>(
        dynamic_cast<ast::Compound*>(goal.substitution.apply(*goal.compound).release()));
    
    // Get matching clauses
    auto matching_clauses = knowledge_base.get_matching_clauses(*applied_goal);
    
    for (const ast::Clause* clause : matching_clauses) {
        // Rename variables in clause to avoid conflicts
        auto renamed_clause = rename_clause_variables(*clause, generate_variable_prefix());
        
        if (auto fact = dynamic_cast<const ast::Fact*>(renamed_clause.get())) {
            // Try to unify with fact
            auto unify_result = unification_engine.unify(*applied_goal, *fact->head);
            
            if (unify_result.success) {
                trace(std::format("Unified with fact: {}", 
                      term_utils::term_to_string(*fact->head)), goal.depth);
                
                // Compose substitutions
                Substitution combined_substitution = goal.substitution.compose(unify_result.substitution);
                
                if (remaining_goals.empty()) {
                    // Found a solution
                    Solution solution;
                    solution.bindings = std::move(combined_substitution);
                    solutions.push_back(std::move(solution));
                    
                    if (!options.find_all_solutions) {
                        return;  // Stop at first solution
                    }
                } else {
                    // Apply substitution to remaining goals and continue
                    auto updated_goals = apply_substitution_to_goals(remaining_goals, combined_substitution);
                    
                    Goal next_goal = std::move(updated_goals[0]);
                    next_goal.depth = goal.depth + 1;
                    
                    std::vector<Goal> rest_goals(updated_goals.begin() + 1, updated_goals.end());
                    resolve_goal(next_goal, rest_goals, solutions);
                }
            }
        } else if (auto rule = dynamic_cast<const ast::Rule*>(renamed_clause.get())) {
            // Try to unify with rule head
            auto unify_result = unification_engine.unify(*applied_goal, *rule->head);
            
            if (unify_result.success) {
                trace(std::format("Unified with rule head: {}", 
                      term_utils::term_to_string(*rule->head)), goal.depth);
                
                // Compose substitutions
                Substitution combined_substitution = goal.substitution.compose(unify_result.substitution);
                
                // Add rule body goals to remaining goals
                std::vector<Goal> new_goals;
                
                // Convert rule body expressions to goals (simplified)
                for (const auto& body_expr : rule->body) {
                    if (auto call_expr = dynamic_cast<const ast::CallExpr*>(body_expr.get())) {
                        auto compound_copy = std::unique_ptr<ast::Compound>(
                            dynamic_cast<ast::Compound*>(term_utils::deep_copy(*call_expr->compound).release()));
                        new_goals.emplace_back(std::move(compound_copy), combined_substitution, goal.depth + 1);
                    }
                }
                
                // Add original remaining goals
                new_goals.insert(new_goals.end(), remaining_goals.begin(), remaining_goals.end());
                
                if (!new_goals.empty()) {
                    Goal next_goal = std::move(new_goals[0]);
                    std::vector<Goal> rest_goals(new_goals.begin() + 1, new_goals.end());
                    resolve_goal(next_goal, rest_goals, solutions);
                } else if (remaining_goals.empty()) {
                    // Rule with empty body - found solution
                    Solution solution;
                    solution.bindings = std::move(combined_substitution);
                    solutions.push_back(std::move(solution));
                }
            }
        }
        
        // Check if we've found enough solutions
        if (solutions.size() >= options.max_solutions) {
            break;
        }
    }
}

std::vector<Goal> QueryResolver::apply_substitution_to_goals(
    const std::vector<Goal>& goals,
    const Substitution& substitution) const {
    
    std::vector<Goal> updated_goals;
    updated_goals.reserve(goals.size());
    
    for (const auto& goal : goals) {
        auto applied_compound = std::unique_ptr<ast::Compound>(
            dynamic_cast<ast::Compound*>(substitution.apply(*goal.compound).release()));
        
        Substitution combined_sub = goal.substitution.compose(substitution);
        updated_goals.emplace_back(std::move(applied_compound), std::move(combined_sub), goal.depth);
    }
    
    return updated_goals;
}

bool QueryResolver::depth_limit_exceeded(size_t depth) const {
    return depth >= options.max_depth;
}

std::unique_ptr<ast::Clause> QueryResolver::rename_clause_variables(
    const ast::Clause& clause,
    const std::string& prefix) const {
    
    // Simplified implementation - in practice would need full variable renaming
    if (auto fact = dynamic_cast<const ast::Fact*>(&clause)) {
        auto renamed_compound = std::unique_ptr<ast::Compound>(
            dynamic_cast<ast::Compound*>(term_utils::deep_copy(*fact->head).release()));
        
        return std::make_unique<ast::Fact>(fact->position, std::move(renamed_compound));
    } else if (auto rule = dynamic_cast<const ast::Rule*>(&clause)) {
        auto renamed_head = std::unique_ptr<ast::Compound>(
            dynamic_cast<ast::Compound*>(term_utils::deep_copy(*rule->head).release()));
        
        auto renamed_rule = std::make_unique<ast::Rule>(rule->position, std::move(renamed_head));
        
        // Copy body (simplified - would need proper renaming)
        for (const auto& body_expr : rule->body) {
            // Simplified copy
        }
        
        return std::move(renamed_rule);
    }
    
    return nullptr;
}

std::string QueryResolver::generate_variable_prefix() const {
    static std::atomic<size_t> counter{0};
    return std::format("_G{}_", counter.fetch_add(1));
}

bool QueryResolver::can_prove(const ast::Compound& goal) const {
    auto solutions = resolve_goal(goal);
    return !solutions.empty();
}

std::optional<Solution> QueryResolver::get_first_solution(const ast::Compound& goal) const {
    ResolverOptions temp_options = options;
    temp_options.find_all_solutions = false;
    temp_options.max_solutions = 1;
    
    QueryResolver temp_resolver(knowledge_base, temp_options);
    auto solutions = temp_resolver.resolve_goal(goal);
    
    if (!solutions.empty()) {
        return solutions[0];
    }
    return std::nullopt;
}

// QueryEngine implementation
void QueryEngine::load_program(const ast::Program& program) {
    for (const auto& clause : program.clauses) {
        if (auto fact = dynamic_cast<const ast::Fact*>(clause.get())) {
            auto fact_copy = std::unique_ptr<ast::Fact>(
                dynamic_cast<ast::Fact*>(term_utils::deep_copy(*fact).release()));
            knowledge_base.add_fact(std::move(fact_copy));
        } else if (auto rule = dynamic_cast<const ast::Rule*>(clause.get())) {
            auto rule_copy = std::unique_ptr<ast::Rule>(
                dynamic_cast<ast::Rule*>(term_utils::deep_copy(*rule).release()));
            knowledge_base.add_rule(std::move(rule_copy));
        }
    }
}

void QueryEngine::add_fact(std::unique_ptr<ast::Fact> fact) {
    knowledge_base.add_fact(std::move(fact));
}

void QueryEngine::add_rule(std::unique_ptr<ast::Rule> rule) {
    knowledge_base.add_rule(std::move(rule));
}

std::vector<Solution> QueryEngine::query(const std::string& query_string) {
    auto parse_result = Parser::parse_source(query_string);
    if (!parse_result) {
        return {};  // Parse error
    }
    
    auto& program = *parse_result.value();
    if (!program.clauses.empty()) {
        if (auto query = dynamic_cast<const ast::Query*>(program.clauses[0].get())) {
            return resolver.resolve_query(*query);
        }
    }
    
    return {};
}

std::vector<Solution> QueryEngine::query(const ast::Query& query) {
    return resolver.resolve_query(query);
}

bool QueryEngine::ask(const std::string& goal_string) {
    // Parse as simple compound term query
    auto parse_result = Parser::parse_source("?- " + goal_string + ".");
    if (!parse_result) {
        return false;
    }
    
    auto& program = *parse_result.value();
    if (!program.clauses.empty()) {
        if (auto query = dynamic_cast<const ast::Query*>(program.clauses[0].get())) {
            auto solutions = resolver.resolve_query(*query);
            return !solutions.empty();
        }
    }
    
    return false;
}

bool QueryEngine::ask(const ast::Compound& goal) {
    return resolver.can_prove(goal);
}

QueryEngine::Stats QueryEngine::get_stats() const {
    return {knowledge_base.fact_count(), knowledge_base.rule_count()};
}

// Solution utilities
namespace solution_utils {

void print_solutions(const std::vector<Solution>& solutions) {
    if (solutions.empty()) {
        std::cout << "false.\n";
        return;
    }
    
    for (size_t i = 0; i < solutions.size(); ++i) {
        std::cout << "Solution " << (i + 1) << ": " << solutions[i].to_string() << std::endl;
    }
}

bool solutions_contain_binding(
    const std::vector<Solution>& solutions,
    const std::string& variable,
    const std::string& value) {
    
    return std::any_of(solutions.begin(), solutions.end(),
        [&](const Solution& sol) {
            return sol.binds(variable) && sol.get_binding_string(variable) == value;
        });
}

std::vector<Solution> filter_solutions_by_binding(
    const std::vector<Solution>& solutions,
    const std::string& variable,
    const std::function<bool(const std::string&)>& predicate) {
    
    std::vector<Solution> filtered;
    
    for (const auto& solution : solutions) {
        if (solution.binds(variable) && 
            predicate(solution.get_binding_string(variable))) {
            filtered.push_back(solution);
        }
    }
    
    return filtered;
}

} // namespace solution_utils

} // namespace logicpp