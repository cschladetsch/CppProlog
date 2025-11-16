#include "parser.hpp"
#include <format>
#include <algorithm>

namespace logicpp {

const Token& Parser::peek(size_t offset) const {
    const size_t index = current_token + offset;
    return (index < tokens.size()) ? tokens[index] : tokens.back();
}

const Token& Parser::advance() {
    if (!at_end()) current_token++;
    return tokens[current_token - 1];
}

bool Parser::check(TokenType type) const {
    return !at_end() && peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& error_msg) {
    if (check(type)) return advance();
    error(error_msg);
}

bool Parser::at_end() const {
    return current_token >= tokens.size() || peek().type == TokenType::EOF_TOKEN;
}

void Parser::synchronize() {
    advance();
    
    while (!at_end()) {
        if (tokens[current_token - 1].type == TokenType::DOT ||
            tokens[current_token - 1].type == TokenType::NEWLINE) {
            return;
        }
        
        switch (peek().type) {
            case TokenType::FACT:
            case TokenType::RULE:
            case TokenType::QUERY_OP:
            case TokenType::TYPE:
            case TokenType::FN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

[[noreturn]] void Parser::error(const std::string& message) {
    Position pos = at_end() ? tokens.back().position : peek().position;
    throw ParseError(message, pos);
}

std::expected<std::unique_ptr<ast::Program>, ParseError> Parser::parse() {
    try {
        return parse_program();
    } catch (const ParseError& e) {
        return std::unexpected(e);
    }
}

std::unique_ptr<ast::Program> Parser::parse_program() {
    auto program = std::make_unique<ast::Program>(Position{});
    
    while (!at_end()) {
        // Skip newlines
        if (match(TokenType::NEWLINE)) continue;
        
        try {
            if (check(TokenType::TYPE)) {
                program->type_definitions.push_back(parse_type_definition());
            } else if (check(TokenType::FN)) {
                program->functions.push_back(parse_function());
            } else {
                program->clauses.push_back(parse_clause());
            }
        } catch (const ParseError&) {
            synchronize();
            // Continue parsing after error
        }
    }
    
    return program;
}

std::unique_ptr<ast::Clause> Parser::parse_clause() {
    if (check(TokenType::QUERY_OP)) {
        return parse_query();
    } else if (check(TokenType::FACT)) {
        return parse_fact();
    } else if (check(TokenType::RULE)) {
        return parse_rule();
    } else {
        // Try to parse as implicit fact
        auto compound = parse_compound();
        consume(TokenType::DOT, "Expected '.' after fact");
        
        return std::make_unique<ast::Fact>(compound->position, std::move(compound));
    }
}

std::unique_ptr<ast::Fact> Parser::parse_fact() {
    const Token& fact_token = consume(TokenType::FACT, "Expected 'fact'");
    auto compound = parse_compound();
    consume(TokenType::DOT, "Expected '.' after fact");
    
    return std::make_unique<ast::Fact>(fact_token.position, std::move(compound));
}

std::unique_ptr<ast::Rule> Parser::parse_rule() {
    const Token& rule_token = consume(TokenType::RULE, "Expected 'rule'");
    auto head = parse_compound();
    consume(TokenType::RULE_OP, "Expected ':-' after rule head");
    
    auto rule = std::make_unique<ast::Rule>(rule_token.position, std::move(head));
    
    // Parse body expressions
    do {
        rule->body.push_back(parse_expression());
    } while (match(TokenType::COMMA));
    
    // Parse optional WHERE clause
    if (match(TokenType::WHERE)) {
        rule->conditions = parse_where_clause();
    }
    
    consume(TokenType::DOT, "Expected '.' after rule");
    
    return rule;
}

std::unique_ptr<ast::Query> Parser::parse_query() {
    const Token& query_token = consume(TokenType::QUERY_OP, "Expected '?-'");
    auto query = std::make_unique<ast::Query>(query_token.position);
    
    do {
        query->goals.push_back(parse_expression());
    } while (match(TokenType::COMMA));
    
    // Parse optional WHERE clause
    if (match(TokenType::WHERE)) {
        query->conditions = parse_where_clause();
    }
    
    consume(TokenType::DOT, "Expected '.' after query");
    
    return query;
}

std::unique_ptr<ast::Expression> Parser::parse_expression() {
    return parse_logical_or();
}

std::unique_ptr<ast::Expression> Parser::parse_logical_or() {
    // For now, just delegate to logical_and
    // TODO: Implement logical OR when we add it to the language
    return parse_logical_and();
}

std::unique_ptr<ast::Expression> Parser::parse_logical_and() {
    // For now, just delegate to equality
    // TODO: Implement logical AND when we add it to the language
    return parse_equality();
}

std::unique_ptr<ast::Expression> Parser::parse_equality() {
    auto expr = parse_comparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        const Token& operator_token = tokens[current_token - 1];
        auto right = parse_comparison();
        
        ast::ComparisonExpr::Op op = (operator_token.type == TokenType::EQUAL) 
            ? ast::ComparisonExpr::Op::EQUAL 
            : ast::ComparisonExpr::Op::NOT_EQUAL;
        
        // Convert expressions to terms if needed
        // This is a simplified approach - real implementation would be more sophisticated
        error("Complex equality expressions not yet implemented");
    }
    
    return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_comparison() {
    auto expr = parse_unification();
    
    while (match({TokenType::LESS, TokenType::GREATER, 
                  TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
        const Token& operator_token = tokens[current_token - 1];
        auto right = parse_unification();
        
        // Convert to comparison expression
        // Simplified for now
        error("Complex comparison expressions not yet implemented");
    }
    
    return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_unification() {
    return parse_primary_expression();
}

std::unique_ptr<ast::Expression> Parser::parse_primary_expression() {
    // Try to parse as compound (predicate call)
    if (check(TokenType::IDENTIFIER)) {
        auto compound = parse_compound();
        return std::make_unique<ast::CallExpr>(compound->position, std::move(compound));
    }
    
    error("Expected expression");
}

std::unique_ptr<ast::Term> Parser::parse_term() {
    if (check(TokenType::IDENTIFIER)) {
        // Could be atom or compound
        if (peek(1).type == TokenType::LPAREN) {
            return parse_compound();
        } else {
            return parse_atom();
        }
    } else if (check(TokenType::UNDERSCORE) || 
               (check(TokenType::IDENTIFIER) && std::isupper(peek().value[0]))) {
        return parse_variable();
    } else if (check(TokenType::INTEGER) || check(TokenType::FLOAT)) {
        return parse_number();
    } else if (check(TokenType::STRING)) {
        return parse_string();
    }
    
    error("Expected term");
}

std::unique_ptr<ast::Compound> Parser::parse_compound() {
    const Token& name_token = consume(TokenType::IDENTIFIER, "Expected identifier");
    auto compound = std::make_unique<ast::Compound>(name_token.position, name_token.value);
    
    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            do {
                compound->arguments.push_back(parse_term());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')' after arguments");
    }
    
    return compound;
}

std::unique_ptr<ast::Atom> Parser::parse_atom() {
    const Token& token = consume(TokenType::IDENTIFIER, "Expected atom");
    return std::make_unique<ast::Atom>(token.position, token.value);
}

std::unique_ptr<ast::Variable> Parser::parse_variable() {
    if (match(TokenType::UNDERSCORE)) {
        const Token& token = tokens[current_token - 1];
        return std::make_unique<ast::Variable>(token.position, "_");
    }
    
    const Token& token = consume(TokenType::IDENTIFIER, "Expected variable");
    // In our language, variables start with uppercase (Prolog convention)
    if (!std::isupper(token.value[0])) {
        error("Variables must start with uppercase letter");
    }
    
    return std::make_unique<ast::Variable>(token.position, token.value);
}

std::unique_ptr<ast::Number> Parser::parse_number() {
    if (match(TokenType::INTEGER)) {
        const Token& token = tokens[current_token - 1];
        int64_t value = std::stoll(token.value);
        return std::make_unique<ast::Number>(token.position, value);
    }
    
    if (match(TokenType::FLOAT)) {
        const Token& token = tokens[current_token - 1];
        double value = std::stod(token.value);
        return std::make_unique<ast::Number>(token.position, value);
    }
    
    error("Expected number");
}

std::unique_ptr<ast::String> Parser::parse_string() {
    const Token& token = consume(TokenType::STRING, "Expected string");
    return std::make_unique<ast::String>(token.position, token.value);
}

std::unique_ptr<ast::Type> Parser::parse_type() {
    if (match(TokenType::IDENTIFIER)) {
        const Token& token = tokens[current_token - 1];
        
        if (token.value == "atom") {
            return std::make_unique<ast::AtomType>(token.position);
        } else if (token.value == "i32" || token.value == "i64") {
            return std::make_unique<ast::IntegerType>(token.position);
        } else {
            // Compound type
            auto compound_type = std::make_unique<ast::CompoundType>(token.position, token.value);
            
            // Parse generic parameters if present
            if (match(TokenType::LESS)) {
                do {
                    compound_type->parameters.push_back(parse_type());
                } while (match(TokenType::COMMA));
                consume(TokenType::GREATER, "Expected '>' after type parameters");
            }
            
            return compound_type;
        }
    }
    
    error("Expected type");
}

std::unique_ptr<ast::TypeDefinition> Parser::parse_type_definition() {
    const Token& type_token = consume(TokenType::TYPE, "Expected 'type'");
    const Token& name_token = consume(TokenType::IDENTIFIER, "Expected type name");
    consume(TokenType::UNIFY, "Expected '=' after type name");
    
    auto type = parse_type();
    consume(TokenType::DOT, "Expected '.' after type definition");
    
    return std::make_unique<ast::TypeDefinition>(
        type_token.position, name_token.value, std::move(type));
}

std::unique_ptr<ast::Function> Parser::parse_function() {
    const Token& fn_token = consume(TokenType::FN, "Expected 'fn'");
    const Token& name_token = consume(TokenType::IDENTIFIER, "Expected function name");
    
    auto function = std::make_unique<ast::Function>(fn_token.position, name_token.value);
    
    // Parse parameters
    consume(TokenType::LPAREN, "Expected '(' after function name");
    if (!check(TokenType::RPAREN)) {
        do {
            const Token& param_name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            auto param_type = parse_type();
            
            function->parameters.emplace_back(param_name.value, std::move(param_type));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    // Parse return type
    consume(TokenType::ARROW, "Expected '->' after parameters");
    function->return_type = parse_type();
    
    // Parse body
    consume(TokenType::LBRACE, "Expected '{' after function signature");
    while (!check(TokenType::RBRACE) && !at_end()) {
        function->body.push_back(parse_expression());
        if (!match(TokenType::COMMA)) break;
    }
    consume(TokenType::RBRACE, "Expected '}' after function body");
    
    return function;
}

std::vector<std::unique_ptr<ast::Expression>> Parser::parse_where_clause() {
    std::vector<std::unique_ptr<ast::Expression>> conditions;
    
    do {
        conditions.push_back(parse_expression());
    } while (match(TokenType::COMMA));
    
    return conditions;
}

// Static utility method
std::expected<std::unique_ptr<ast::Program>, ParseError> 
Parser::parse_source(std::string_view source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    Parser parser(std::move(tokens));
    return parser.parse();
}

} // namespace logicpp