#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include "lexer.hpp"

namespace logicpp::ast {

// Forward declarations
struct Expression;
struct Term;
struct Atom;
struct Variable;
struct Compound;
struct Number;
struct String;

// Base AST node
struct ASTNode {
    Position position;
    virtual ~ASTNode() = default;
    
protected:
    ASTNode(Position pos) : position(pos) {}
};

// Type system
struct Type : ASTNode {
    Type(Position pos) : ASTNode(pos) {}
};

struct AtomType : Type {
    AtomType(Position pos) : Type(pos) {}
};

struct IntegerType : Type {
    IntegerType(Position pos) : Type(pos) {}
};

struct CompoundType : Type {
    std::string name;
    std::vector<std::unique_ptr<Type>> parameters;
    
    CompoundType(Position pos, std::string n) 
        : Type(pos), name(std::move(n)) {}
};

// Terms
struct Term : ASTNode {
    std::optional<std::unique_ptr<Type>> type_annotation;
    
    Term(Position pos) : ASTNode(pos) {}
};

struct Atom : Term {
    std::string value;
    
    Atom(Position pos, std::string v) 
        : Term(pos), value(std::move(v)) {}
};

struct Variable : Term {
    std::string name;
    
    Variable(Position pos, std::string n) 
        : Term(pos), name(std::move(n)) {}
};

struct Number : Term {
    std::variant<int64_t, double> value;
    
    Number(Position pos, int64_t v) 
        : Term(pos), value(v) {}
    Number(Position pos, double v) 
        : Term(pos), value(v) {}
};

struct String : Term {
    std::string value;
    
    String(Position pos, std::string v) 
        : Term(pos), value(std::move(v)) {}
};

struct Compound : Term {
    std::string functor;
    std::vector<std::unique_ptr<Term>> arguments;
    
    Compound(Position pos, std::string f) 
        : Term(pos), functor(std::move(f)) {}
};

// Expressions and conditions
struct Expression : ASTNode {
    Expression(Position pos) : ASTNode(pos) {}
};

struct UnificationExpr : Expression {
    std::unique_ptr<Term> left;
    std::unique_ptr<Term> right;
    
    UnificationExpr(Position pos, std::unique_ptr<Term> l, std::unique_ptr<Term> r)
        : Expression(pos), left(std::move(l)), right(std::move(r)) {}
};

struct ComparisonExpr : Expression {
    enum class Op { EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL };
    
    std::unique_ptr<Term> left;
    std::unique_ptr<Term> right;
    Op operation;
    
    ComparisonExpr(Position pos, std::unique_ptr<Term> l, std::unique_ptr<Term> r, Op op)
        : Expression(pos), left(std::move(l)), right(std::move(r)), operation(op) {}
};

struct CallExpr : Expression {
    std::unique_ptr<Compound> compound;
    
    CallExpr(Position pos, std::unique_ptr<Compound> c)
        : Expression(pos), compound(std::move(c)) {}
};

// Clauses
struct Clause : ASTNode {
    Clause(Position pos) : ASTNode(pos) {}
};

struct Fact : Clause {
    std::unique_ptr<Compound> head;
    
    Fact(Position pos, std::unique_ptr<Compound> h)
        : Clause(pos), head(std::move(h)) {}
};

struct Rule : Clause {
    std::unique_ptr<Compound> head;
    std::vector<std::unique_ptr<Expression>> body;
    std::vector<std::unique_ptr<Expression>> conditions; // WHERE clause
    
    Rule(Position pos, std::unique_ptr<Compound> h)
        : Clause(pos), head(std::move(h)) {}
};

struct Query : Clause {
    std::vector<std::unique_ptr<Expression>> goals;
    std::vector<std::unique_ptr<Expression>> conditions; // WHERE clause
    
    Query(Position pos) : Clause(pos) {}
};

// Type definitions
struct TypeDefinition : ASTNode {
    std::string name;
    std::unique_ptr<Type> type;
    
    TypeDefinition(Position pos, std::string n, std::unique_ptr<Type> t)
        : ASTNode(pos), name(std::move(n)), type(std::move(t)) {}
};

// Function definitions (first-class functions)
struct Function : ASTNode {
    std::string name;
    std::vector<std::pair<std::string, std::unique_ptr<Type>>> parameters;
    std::unique_ptr<Type> return_type;
    std::vector<std::unique_ptr<Expression>> body;
    
    Function(Position pos, std::string n)
        : ASTNode(pos), name(std::move(n)) {}
};

// Pattern matching
struct Pattern : ASTNode {
    Pattern(Position pos) : ASTNode(pos) {}
};

struct MatchExpr : Expression {
    std::unique_ptr<Term> target;
    std::vector<std::pair<std::unique_ptr<Pattern>, std::vector<std::unique_ptr<Expression>>>> arms;
    
    MatchExpr(Position pos, std::unique_ptr<Term> t)
        : Expression(pos), target(std::move(t)) {}
};

// Program structure
struct Program : ASTNode {
    std::vector<std::unique_ptr<Clause>> clauses;
    std::vector<std::unique_ptr<TypeDefinition>> type_definitions;
    std::vector<std::unique_ptr<Function>> functions;
    
    Program(Position pos) : ASTNode(pos) {}
};

// Visitor pattern for AST traversal
template<typename T>
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual T visit_program(const Program& node) = 0;
    virtual T visit_fact(const Fact& node) = 0;
    virtual T visit_rule(const Rule& node) = 0;
    virtual T visit_query(const Query& node) = 0;
    virtual T visit_atom(const Atom& node) = 0;
    virtual T visit_variable(const Variable& node) = 0;
    virtual T visit_compound(const Compound& node) = 0;
    virtual T visit_number(const Number& node) = 0;
    virtual T visit_string(const String& node) = 0;
    virtual T visit_unification(const UnificationExpr& node) = 0;
    virtual T visit_comparison(const ComparisonExpr& node) = 0;
    virtual T visit_call(const CallExpr& node) = 0;
    virtual T visit_match(const MatchExpr& node) = 0;
};

} // namespace logicpp::ast