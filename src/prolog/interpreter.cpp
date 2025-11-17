#include "interpreter.h"
#include "builtin_predicates.h"
#include <rang.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace prolog {

Interpreter::Interpreter(bool interactive) 
    : database_(std::make_unique<Database>()),
      resolver_(std::make_unique<Resolver>(*database_)),
      interactive_mode_(interactive) {
    BuiltinPredicates::registerBuiltins();
}

void Interpreter::run() {
    if (!interactive_mode_) return;
    
    std::cout << rang::style::bold << rang::fg::cyan 
              << "CppLProlog Interpreter v1.0" << rang::style::reset << "\n";
    std::cout << "Type " << rang::fg::yellow << ":help" << rang::style::reset 
              << " for commands, or enter Prolog queries.\n\n";
    
    while (true) {
        std::string input = readInput("?- ");
        
        if (input.empty()) continue;
        
        if (input == ":quit" || input == ":q") {
            break;
        }
        
        try {
            if (isCommand(input)) {
                handleCommand(input);
            } else if (input.back() == '.') {
                // Looks like a clause, add to database
                database_->loadProgram(input);
                std::cout << "Clause added.\n";
            } else {
                // Treat as query
                queryInteractive(input);
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
        
        std::cout << "\n";
    }
    
    std::cout << "Goodbye!\n";
}

void Interpreter::loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    
    database_->loadProgram(content.str());
}

void Interpreter::loadString(const std::string& program) {
    database_->loadProgram(program);
}

std::vector<Solution> Interpreter::query(const std::string& query_string) {
    Parser parser({});
    TermPtr query = parser.parseQuery(query_string);
    
    return resolver_->solve(query);
}

void Interpreter::queryInteractive(const std::string& query_string) {
    try {
        auto solutions = query(query_string);
        printSolutions(solutions);
    } catch (const std::exception& e) {
        std::cout << "Query error: " << e.what() << "\n";
    }
}

void Interpreter::showHelp() const {
    std::cout << "Commands:\n";
    std::cout << "  :help, :h     - Show this help\n";
    std::cout << "  :quit, :q     - Exit interpreter\n";
    std::cout << "  :load <file>  - Load Prolog file\n";
    std::cout << "  :clear        - Clear database\n";
    std::cout << "  :list         - List all clauses\n";
    std::cout << "  :stats        - Show statistics\n";
    std::cout << "\nQuery examples:\n";
    std::cout << "  parent(tom, bob).\n";
    std::cout << "  parent(X, bob).\n";
}

void Interpreter::showStatistics() const {
    std::cout << "Database statistics:\n";
    std::cout << "  Clauses: " << database_->size() << "\n";
}

void Interpreter::processCommand(const std::string& input) {
    if (isCommand(input)) {
        handleCommand(input);
    }
}

bool Interpreter::isCommand(const std::string& input) const {
    return !input.empty() && input[0] == ':';
}

void Interpreter::handleCommand(const std::string& command) {
    if (command == ":help" || command == ":h") {
        showHelp();
    } else if (command == ":clear") {
        database_->clear();
        std::cout << "Database cleared.\n";
    } else if (command == ":list") {
        std::cout << database_->toString();
    } else if (command == ":stats") {
        showStatistics();
    } else if (command.substr(0, 5) == ":load") {
        if (command.length() > 6) {
            std::string filename = command.substr(6);
            loadFile(filename);
            std::cout << "Loaded file: " << filename << "\n";
        } else {
            std::cout << "Usage: :load <filename>\n";
        }
    } else {
        std::cout << "Unknown command: " << command << "\n";
        std::cout << "Type :help for available commands.\n";
    }
}

#include "interpreter.h"
#include "builtin_predicates.h"
#include <rang.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include "linenoise-ng/linenoise.hpp"

namespace prolog {

    // ... (rest of the file remains the same)

    void Interpreter::handleCommand(const std::string& command) {
    // ... (existing implementation)
    }

    std::string Interpreter::readInput(const std::string& prompt) const {
    static linenoise::LineNoise ln;
    static bool first_run = true;
    if (first_run) {
        first_run = false;
        const auto path = "~/.cpp_prolog_history";
        ln.SetHistory(path);
        ln.SetKeyMap(linenoise::KeyMap::Vi);
        ln.SetMultiLine(true);
        ln.SetSyntaxHighlighting(true);
        ln.SetSyntaxHighlighter([](const std::string& line) {
            std::stringstream ss;
            // Basic highlighting: keywords, literals, comments
            // This is a simple example, a more robust solution would use a proper tokenizer
            std::string current_word;
            for (char c : line) {
                if (std::isspace(c) || std::string("(),.").find(c) != std::string::npos) {
                    if (!current_word.empty()) {
                        if (current_word == ":-" || current_word == "is" || current_word == "not") {
                            ss << rang::fg::magenta << current_word << rang::fg::reset;
                        } else if (isupper(current_word[0])) {
                            ss << rang::fg::yellow << current_word << rang::fg::reset;
                        } else {
                            ss << rang::fg::cyan << current_word << rang::fg::reset;
                        }
                        current_word.clear();
                    }
                    ss << c;
                } else {
                    current_word += c;
                }
            }
            if (!current_word.empty()) {
                 if (isupper(current_word[0])) {
                    ss << rang::fg::yellow << current_word << rang::fg::reset;
                } else {
                    ss << rang::fg::cyan << current_word << rang::fg::reset;
                }
            }
            return ss.str();
        });
    }

    std::string input;
    std::stringstream prompt_ss;
    prompt_ss << rang::fg::green << prompt << rang::fg::reset;
    auto quit = ln.GetLine(prompt_ss.str().c_str(), input);

    if (quit) {
        return ":quit";
    }

    ln.AddHistory(input.c_str());
    return input;
}

    void Interpreter::printSolutions(const std::vector<Solution>& solutions) const {
    // ... (existing implementation)
    }
}

void Interpreter::printSolutions(const std::vector<Solution>& solutions) const {
    if (solutions.empty()) {
        std::cout << "false.\n";
    } else {
        for (size_t i = 0; i < solutions.size(); ++i) {
            std::cout << solutions[i].toString();
            if (i < solutions.size() - 1) {
                std::cout << " ;";
            }
            std::cout << "\n";
        }
        if (solutions.size() == 1 && solutions[0].bindings.empty()) {
            std::cout << "true.\n";
        }
    }
}

}