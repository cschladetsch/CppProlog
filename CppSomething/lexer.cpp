#include "lexer.hpp"
#include <cctype>
#include <iostream>

namespace logicpp {

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"fact", TokenType::FACT},
    {"rule", TokenType::RULE},
    {"type", TokenType::TYPE},
    {"fn", TokenType::FN},
    {"match", TokenType::MATCH},
    {"where", TokenType::WHERE},
    {"let", TokenType::LET}
};

char Lexer::peek(size_t offset) const noexcept {
    const size_t pos = current_pos + offset;
    return (pos < source.length()) ? source[pos] : '\0';
}

char Lexer::advance() noexcept {
    if (at_end()) return '\0';
    
    const char ch = source[current_pos++];
    if (ch == '\n') {
        position.line++;
        position.column = 1;
    } else {
        position.column++;
    }
    position.offset++;
    return ch;
}

void Lexer::skip_whitespace() noexcept {
    while (!at_end()) {
        const char ch = peek();
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            advance();
        } else if (ch == '/' && peek(1) == '/') {
            skip_comment();
        } else {
            break;
        }
    }
}

void Lexer::skip_comment() noexcept {
    // Skip until end of line
    while (!at_end() && peek() != '\n') {
        advance();
    }
}

Token Lexer::make_token(TokenType type, std::string value) const {
    return Token(type, std::move(value), position);
}

Token Lexer::scan_identifier() {
    const size_t start_pos = current_pos - 1; // We already consumed first char
    
    while (!at_end()) {
        const char ch = peek();
        if (std::isalnum(ch) || ch == '_') {
            advance();
        } else {
            break;
        }
    }
    
    std::string value(source.substr(start_pos, current_pos - start_pos));
    
    // Check if it's a keyword
    if (auto it = keywords.find(value); it != keywords.end()) {
        return make_token(it->second, std::move(value));
    }
    
    return make_token(TokenType::IDENTIFIER, std::move(value));
}

Token Lexer::scan_number() {
    const size_t start_pos = current_pos - 1;
    bool is_float = false;
    
    while (!at_end() && std::isdigit(peek())) {
        advance();
    }
    
    // Check for decimal point
    if (!at_end() && peek() == '.' && std::isdigit(peek(1))) {
        is_float = true;
        advance(); // consume '.'
        while (!at_end() && std::isdigit(peek())) {
            advance();
        }
    }
    
    std::string value(source.substr(start_pos, current_pos - start_pos));
    TokenType type = is_float ? TokenType::FLOAT : TokenType::INTEGER;
    
    return make_token(type, std::move(value));
}

Token Lexer::scan_string() {
    const size_t start_pos = current_pos;
    
    while (!at_end()) {
        const char ch = peek();
        if (ch == '"') {
            break;
        } else if (ch == '\\') {
            advance(); // Skip escape character
            if (!at_end()) advance(); // Skip escaped character
        } else {
            advance();
        }
    }
    
    if (at_end()) {
        return make_token(TokenType::INVALID, "Unterminated string");
    }
    
    std::string value(source.substr(start_pos, current_pos - start_pos));
    advance(); // Consume closing quote
    
    return make_token(TokenType::STRING, std::move(value));
}

Token Lexer::scan_operator() {
    const char ch1 = peek(-1); // We already advanced
    const char ch2 = peek();
    
    // Two-character operators
    if (ch2 != '\0') {
        std::string two_char = std::string(1, ch1) + ch2;
        
        if (two_char == ":-") {
            advance();
            return make_token(TokenType::RULE_OP, ":-");
        } else if (two_char == "?-") {
            advance();
            return make_token(TokenType::QUERY_OP, "?-");
        } else if (two_char == "->") {
            advance();
            return make_token(TokenType::ARROW, "->");
        } else if (two_char == "\\=") {
            advance();
            return make_token(TokenType::NOT_UNIFY, "\\=");
        } else if (two_char == "<=") {
            advance();
            return make_token(TokenType::LESS_EQUAL, "<=");
        } else if (two_char == ">=") {
            advance();
            return make_token(TokenType::GREATER_EQUAL, ">=");
        } else if (two_char == "==") {
            advance();
            return make_token(TokenType::EQUAL, "==");
        } else if (two_char == "!=") {
            advance();
            return make_token(TokenType::NOT_EQUAL, "!=");
        }
    }
    
    // Single-character operators and delimiters
    switch (ch1) {
        case '=': return make_token(TokenType::UNIFY, "=");
        case '<': return make_token(TokenType::LESS, "<");
        case '>': return make_token(TokenType::GREATER, ">");
        case '|': return make_token(TokenType::PIPE, "|");
        case '(': return make_token(TokenType::LPAREN, "(");
        case ')': return make_token(TokenType::RPAREN, ")");
        case '{': return make_token(TokenType::LBRACE, "{");
        case '}': return make_token(TokenType::RBRACE, "}");
        case '[': return make_token(TokenType::LBRACKET, "[");
        case ']': return make_token(TokenType::RBRACKET, "]");
        case '.': return make_token(TokenType::DOT, ".");
        case ',': return make_token(TokenType::COMMA, ",");
        case ';': return make_token(TokenType::SEMICOLON, ";");
        case ':': return make_token(TokenType::COLON, ":");
        case '_': return make_token(TokenType::UNDERSCORE, "_");
        default:
            return make_token(TokenType::INVALID, 
                            std::format("Unexpected character: '{}'", ch1));
    }
}

Token Lexer::next_token() {
    skip_whitespace();
    
    if (at_end()) {
        return make_token(TokenType::EOF_TOKEN, "");
    }
    
    const char ch = advance();
    
    if (ch == '\n') {
        return make_token(TokenType::NEWLINE, "\n");
    }
    
    if (std::isalpha(ch) || ch == '_') {
        return scan_identifier();
    }
    
    if (std::isdigit(ch)) {
        return scan_number();
    }
    
    if (ch == '"') {
        return scan_string();
    }
    
    return scan_operator();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (true) {
        Token token = next_token();
        const bool is_eof = (token.type == TokenType::EOF_TOKEN);
        
        tokens.push_back(std::move(token));
        
        if (is_eof) break;
    }
    
    return tokens;
}

std::string to_string(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::ATOM: return "ATOM";
        case TokenType::FACT: return "FACT";
        case TokenType::RULE: return "RULE";
        case TokenType::QUERY: return "QUERY";
        case TokenType::TYPE: return "TYPE";
        case TokenType::FN: return "FN";
        case TokenType::MATCH: return "MATCH";
        case TokenType::WHERE: return "WHERE";
        case TokenType::LET: return "LET";
        case TokenType::RULE_OP: return "RULE_OP";
        case TokenType::QUERY_OP: return "QUERY_OP";
        case TokenType::ARROW: return "ARROW";
        case TokenType::PIPE: return "PIPE";
        case TokenType::UNIFY: return "UNIFY";
        case TokenType::NOT_UNIFY: return "NOT_UNIFY";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::DOT: return "DOT";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::UNDERSCORE: return "UNDERSCORE";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    return os << std::format("{}({}) at {}:{}", 
                           to_string(token.type), 
                           token.value,
                           token.position.line,
                           token.position.column);
}

} // namespace logicpp