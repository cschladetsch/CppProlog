#include "database.h"
#include "parser.h"
#include <sstream>

namespace prolog {

void Database::addClause(ClausePtr clause) {
    std::string key = extractFunctorArity(clause->head());
    index_[key].push_back(clauses_.size());
    clauses_.push_back(std::move(clause));
}

void Database::addFact(TermPtr head) {
    addClause(makeFact(std::move(head)));
}

void Database::addRule(TermPtr head, TermList body) {
    addClause(makeRule(std::move(head), std::move(body)));
}

std::vector<ClausePtr> Database::findClauses(const std::string& functor, size_t arity) const {
    std::string key = makeKey(functor, arity);
    auto it = index_.find(key);
    
    std::vector<ClausePtr> result;
    if (it != index_.end()) {
        result.reserve(it->second.size());
        for (size_t index : it->second) {
            result.push_back(clauses_[index]->clone());
        }
    }
    
    return result;
}

std::vector<ClausePtr> Database::findMatchingClauses(const TermPtr& goal) const {
    std::string key = extractFunctorArity(goal);
    auto it = index_.find(key);
    
    std::vector<ClausePtr> result;
    if (it != index_.end()) {
        result.reserve(it->second.size());
        for (size_t index : it->second) {
            result.push_back(clauses_[index]->clone());
        }
    }
    
    return result;
}

void Database::clear() {
    clauses_.clear();
    index_.clear();
}

void Database::loadProgram(const std::string& program) {
    try {
        Parser parser({});
        auto clauses = parser.parseProgram(program);
        
        for (auto& clause : clauses) {
            addClause(std::move(clause));
        }
    } catch (const ParseException& e) {
        throw std::runtime_error("Failed to load program: " + std::string(e.what()));
    }
}

std::string Database::toString() const {
    std::ostringstream oss;
    for (const auto& clause : clauses_) {
        oss << clause->toString() << "\n";
    }
    return oss.str();
}

std::string Database::makeKey(const std::string& functor, size_t arity) const {
    return functor + "/" + std::to_string(arity);
}

std::string Database::extractFunctorArity(const TermPtr& term) const {
    switch (term->type()) {
        case TermType::ATOM: {
            auto atom = term->as<Atom>();
            return makeKey(atom->name(), 0);
        }
        case TermType::COMPOUND: {
            auto compound = term->as<Compound>();
            return makeKey(compound->functor(), compound->arity());
        }
        default:
            return "";
    }
}

}