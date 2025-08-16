#pragma once

#include "term.h"
#include "clause.h"
#include "database.h"
#include "unification.h"
#include "solution.h"
#include "builtin_predicates.h"
#include <vector>
#include <functional>
#include <memory>

namespace prolog {

class Choice {
public:
    TermPtr goal;
    TermList remaining_goals;
    std::vector<ClausePtr> clauses;
    size_t clause_index;
    Substitution bindings;
    
    Choice(TermPtr g, TermList rg, std::vector<ClausePtr> cs, Substitution b)
        : goal(std::move(g)), remaining_goals(std::move(rg)), 
          clauses(std::move(cs)), clause_index(0), bindings(std::move(b)) {}
    
    bool hasMoreChoices() const {
        return clause_index < clauses.size();
    }
    
    ClausePtr nextClause() {
        if (hasMoreChoices()) {
            return std::move(clauses[clause_index++]);
        }
        return nullptr;
    }
};

class Resolver {
private:
    const Database& database_;
    std::vector<Choice> choice_stack_;
    size_t max_depth_;
    size_t current_depth_;
    bool termination_requested_;
    
public:
    explicit Resolver(const Database& db, size_t max_depth = 1000) 
        : database_(db), max_depth_(max_depth), current_depth_(0), termination_requested_(false) {}
    
    std::vector<Solution> solve(const TermPtr& query);
    std::vector<Solution> solve(const TermList& goals);
    
    void solveWithCallback(const TermPtr& query, 
                          std::function<bool(const Solution&)> callback);
    void solveWithCallback(const TermList& goals, 
                          std::function<bool(const Solution&)> callback);
    
private:
    bool solveGoals(const TermList& goals, const Substitution& bindings, 
                   std::function<bool(const Solution&)> callback);
    
    void pushChoice(TermPtr goal, TermList remaining_goals, 
                   std::vector<ClausePtr> clauses, Substitution bindings);
    bool backtrack();
    
    std::string renameVariables(size_t clause_id) const;
    
    // Helper function to collect variables from a term
    void collectVariablesFromTerm(const TermPtr& term, std::vector<std::string>& variables) const;
    
    // Helper function to filter bindings to only include query variables
    Substitution filterBindings(const Substitution& bindings, 
                               const std::vector<std::string>& queryVariables) const;
};

}