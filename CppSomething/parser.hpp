#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include <memory>
#include <vector>
#include <expected>
#include <string>

namespace logicpp {

class ParseError : public std::exception {
private:
    std::string message;
    Position position;

public:
    ParseError(std::string msg, Position pos) 
        : message(std::move(msg)), position(pos) {}
    
    const char* what() const noexcept override { return message.c_str(); }
    Position where() const noexcept { return position; }
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t current_token = 0;
    
    // Token management
    const Token& peek(size_t offset = 0) const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& error_msg);
    bool at_end() const;
    
    // Error handling
    void synchronize();
    [[noreturn]] void error(const std::string& message);
    
    // Parsing methods
    std::unique_ptr<ast::Program> parse_program();
    std::unique_ptr<ast::Clause> parse_clause();
    std::unique_ptr<ast::Fact> parse_fact();
    std::unique_ptr<ast::Rule> parse_rule();
    std::unique_ptr<ast::Query> parse_query();
    std::unique_ptr<ast::TypeDefinition> parse_type_definition();
    std::unique_ptr<ast::Function> parse_function();
    
    // Expression parsing
    std::unique_ptr<ast::Expression> parse_expression();
    std::unique_ptr<ast::Expression> parse_logical_or();
    std::unique_ptr<ast::Expression> parse_logical_and();
    std::unique_ptr<ast::Expression> parse_equality();
    std::unique_ptr<ast::Expression> parse_comparison();
    std::unique_ptr<ast::Expression> parse_unification();
    std::unique_ptr<ast::Expression> parse_primary_expression();
    
    // Term parsing
    std::unique_ptr<ast::Term> parse_term();
    std::unique_ptr<ast::Compound> parse_compound();
    std::unique_ptr<ast::Atom> parse_atom();
    std::unique_ptr<ast::Variable> parse_variable();
    std::unique_ptr<ast::Number> parse_number();
    std::unique_ptr<ast::String> parse_string();
    
    // Type parsing
    std::unique_ptr<ast::Type> parse_type();
    
    // Utility methods
    std::vector<std::unique_ptr<ast::Expression>> parse_where_clause();
    std::vector<std::unique_ptr<ast::Term>> parse_argument_list();
    
public:
    explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}
    
    std::expected<std::unique_ptr<ast::Program>, ParseError> parse();
    
    // Static utility for quick parsing
    static std::expected<std::unique_ptr<ast::Program>, ParseError> 
    parse_source(std::string_view source);
};

} // namespace logicpp