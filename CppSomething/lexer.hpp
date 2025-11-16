#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <span>
#include <format>

namespace logicpp {

enum class TokenType {
    // Literals
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING,
    ATOM,

    // Keywords
    FACT,
    RULE,
    QUERY,
    TYPE,
    FN,
    MATCH,
    WHERE,
    LET,

    // Operators
    RULE_OP,        // :-
    QUERY_OP,       // ?-
    ARROW,          // ->
    PIPE,           // |
    UNIFY,          // =
    NOT_UNIFY,      // \=
    LESS,           // <
    GREATER,        // >
    LESS_EQUAL,     // <=
    GREATER_EQUAL,  // >=
    EQUAL,          // ==
    NOT_EQUAL,      // !=

    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    DOT,            // .
    COMMA,          // ,
    SEMICOLON,      // ;
    COLON,          // :
    UNDERSCORE,     // _

    // Special
    NEWLINE,
    EOF_TOKEN,
    INVALID
};

struct Position {
    size_t line = 1;
    size_t column = 1;
    size_t offset = 0;
};

struct Token {
    TokenType type;
    std::string value;
    Position position;

    Token(TokenType t, std::string v, Position pos) 
        : type(t), value(std::move(v)), position(pos) {}
};

class Lexer {
private:
    std::string_view source;
    size_t current_pos = 0;
    Position position;
    
    static const std::unordered_map<std::string, TokenType> keywords;

    char peek(size_t offset = 0) const noexcept;
    char advance() noexcept;
    void skip_whitespace() noexcept;
    void skip_comment() noexcept;
    
    Token make_token(TokenType type, std::string value) const;
    Token scan_identifier();
    Token scan_number();
    Token scan_string();
    Token scan_operator();

public:
    explicit Lexer(std::string_view source) : source(source) {}
    
    std::vector<Token> tokenize();
    Token next_token();
    
    // Utility methods
    bool at_end() const noexcept { return current_pos >= source.length(); }
    Position current_position() const noexcept { return position; }
};

// Token type to string conversion
std::string to_string(TokenType type);
std::ostream& operator<<(std::ostream& os, const Token& token);

} // namespace logicpp