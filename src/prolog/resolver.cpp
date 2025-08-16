#include "resolver.h"
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <unordered_set>

namespace prolog {

std::vector<Solution> Resolver::solve(const TermPtr& query) {
    // Collect variables from the query
    std::vector<std::string> queryVariables;
    collectVariablesFromTerm(query, queryVariables);
    
    std::vector<Solution> solutions;
    
    solveWithCallback({query}, [&solutions, this, &queryVariables](const Solution& solution) {
        // Filter bindings to only include query variables
        Solution filteredSolution{filterBindings(solution.bindings, queryVariables)};
        solutions.push_back(filteredSolution);
        return true; // Continue to find more solutions
    });
    
    return solutions;
}

std::vector<Solution> Resolver::solve(const TermList& goals) {
    // Collect variables from all goals
    std::vector<std::string> queryVariables;
    for (const auto& goal : goals) {
        collectVariablesFromTerm(goal, queryVariables);
    }
    
    std::vector<Solution> solutions;
    
    solveWithCallback(goals, [&solutions, this, &queryVariables](const Solution& solution) {
        // Filter bindings to only include query variables
        Solution filteredSolution{filterBindings(solution.bindings, queryVariables)};
        solutions.push_back(filteredSolution);
        return true; // Continue to find more solutions
    });
    
    return solutions;
}

void Resolver::solveWithCallback(const TermPtr& query, 
                                std::function<bool(const Solution&)> callback) {
    choice_stack_.clear();
    current_depth_ = 0;
    
    Substitution empty_bindings;
    solveGoals({query}, empty_bindings, callback);
}

void Resolver::solveWithCallback(const TermList& goals, 
                                std::function<bool(const Solution&)> callback) {
    choice_stack_.clear();
    current_depth_ = 0;
    
    Substitution empty_bindings;
    solveGoals(goals, empty_bindings, callback);
}

bool Resolver::solveGoals(const TermList& goals, const Substitution& bindings, 
                         std::function<bool(const Solution&)> callback) {
    if (current_depth_ > max_depth_) {
        return false;
    }
    
    if (goals.empty()) {
        Solution solution{bindings};
        return callback(solution);
    }
    
    TermPtr current_goal = Unification::applySubstitution(goals[0], bindings);
    TermList remaining_goals(goals.begin() + 1, goals.end());
    Unification::applySubstitutionInPlace(remaining_goals, bindings);
    
    auto matching_clauses = database_.findMatchingClauses(current_goal);
    
    if (matching_clauses.empty()) {
        return false;
    }
    
    bool found_any = false;
    
    // Try each matching clause
    for (auto& clause : matching_clauses) {
        current_depth_++;
        
        if (current_depth_ > max_depth_) {
            current_depth_--;
            break;
        }
        
        // Rename clause variables to avoid conflicts
        std::string suffix = "_" + std::to_string(current_depth_) + "_" + std::to_string(rand());
        ClausePtr renamed_clause = clause->rename(suffix);
        
        // Try to unify goal with clause head
        auto unification_result = Unification::unify(current_goal, renamed_clause->head());
        
        if (unification_result) {
            // Unification succeeded
            Substitution new_bindings = Unification::compose(bindings, *unification_result);
            
            // Create new goals: remaining goals + clause body
            TermList new_goals = remaining_goals;
            for (const auto& body_goal : renamed_clause->body()) {
                new_goals.push_back(Unification::applySubstitution(body_goal, *unification_result));
            }
            
            // Recursively solve new goals
            bool continue_search = solveGoals(new_goals, new_bindings, callback);
            if (continue_search) {
                found_any = true;
                // Continue to find more solutions
            } else {
                // Callback returned false, stop searching
                found_any = true; // We found at least one solution
                current_depth_--;
                return false; // Stop the search
            }
        }
        
        current_depth_--;
    }
    
    return found_any;
}

void Resolver::pushChoice(TermPtr goal, TermList remaining_goals, 
                         std::vector<ClausePtr> clauses, Substitution bindings) {
    choice_stack_.emplace_back(std::move(goal), std::move(remaining_goals), 
                              std::move(clauses), std::move(bindings));
}

bool Resolver::backtrack() {
    // This method is no longer used in the simplified implementation
    return false;
}

std::string Resolver::renameVariables(size_t clause_id) const {
    return "_" + std::to_string(clause_id) + "_" + std::to_string(current_depth_);
}

void Resolver::collectVariablesFromTerm(const TermPtr& term, std::vector<std::string>& variables) const {
    std::unordered_set<std::string> seen;
    
    std::function<void(const TermPtr&)> collectFromTerm = [&](const TermPtr& t) {
        switch (t->type()) {
            case TermType::VARIABLE: {
                auto var = t->as<Variable>();
                if (seen.insert(var->name()).second) {
                    variables.push_back(var->name());
                }
                break;
            }
            case TermType::COMPOUND: {
                auto compound = t->as<Compound>();
                for (const auto& arg : compound->arguments()) {
                    collectFromTerm(arg);
                }
                break;
            }
            case TermType::LIST: {
                auto list = t->as<List>();
                for (const auto& elem : list->elements()) {
                    collectFromTerm(elem);
                }
                if (list->tail()) {
                    collectFromTerm(list->tail());
                }
                break;
            }
            default:
                break;
        }
    };
    
    collectFromTerm(term);
}

Substitution Resolver::filterBindings(const Substitution& bindings, 
                                     const std::vector<std::string>& queryVariables) const {
    Substitution filtered;
    for (const auto& varName : queryVariables) {
        auto it = bindings.find(varName);
        if (it != bindings.end()) {
            filtered[varName] = it->second;
        }
    }
    return filtered;
}

}