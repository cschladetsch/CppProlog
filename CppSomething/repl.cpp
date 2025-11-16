#include "repl.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <format>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace logicpp {

// REPLInputParser implementation
REPLInput REPLInputParser::parse(const std::string& input) {
    std::string trimmed = trim(input);
    
    if (trimmed.empty()) {
        return REPLInput("Empty input");
    }
    
    REPLCommand cmd = classify_input(trimmed);
    
    switch (cmd) {
        case REPLCommand::HELP:
        case REPLCommand::QUIT:
        case REPLCommand::CLEAR:
        case REPLCommand::STATS:
            return REPLInput(cmd, "");
            
        case REPLCommand::LOAD:
        case REPLCommand::SAVE:
        case REPLCommand::TRACE: {
            size_t space_pos = trimmed.find(' ');
            if (space_pos == std::string::npos) {
                return REPLInput("Command requires an argument");
            }
            std::string args = trim(trimmed.substr(space_pos + 1));
            return REPLInput(cmd, args);
        }
        
        case REPLCommand::QUERY:
        case REPLCommand::FACT:
        case REPLCommand::RULE:
        case REPLCommand::TYPE:
        case REPLCommand::FUNCTION:
            return REPLInput(cmd, trimmed);
            
        default:
            return REPLInput("Unknown command");
    }
}

REPLCommand REPLInputParser::classify_input(const std::string& input) {
    if (input.starts_with(":help")) return REPLCommand::HELP;
    if (input.starts_with(":quit") || input.starts_with(":q")) return REPLCommand::QUIT;
    if (input.starts_with(":load")) return REPLCommand::LOAD;
    if (input.starts_with(":save")) return REPLCommand::SAVE;
    if (input.starts_with(":clear")) return REPLCommand::CLEAR;
    if (input.starts_with(":trace")) return REPLCommand::TRACE;
    if (input.starts_with(":stats")) return REPLCommand::STATS;
    
    if (input.starts_with("?-")) return REPLCommand::QUERY;
    if (input.starts_with("fact ")) return REPLCommand::FACT;
    if (input.starts_with("rule ")) return REPLCommand::RULE;
    if (input.starts_with("type ")) return REPLCommand::TYPE;
    if (input.starts_with("fn ")) return REPLCommand::FUNCTION;
    
    // Try to detect implicit fact (compound term ending with .)
    if (input.ends_with(".") && input.find(":-") == std::string::npos) {
        return REPLCommand::FACT;
    }
    
    return REPLCommand::UNKNOWN;
}

std::string REPLInputParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// REPL implementation
REPL::REPL() {
    state.resolver_options.trace_execution = state.tracing_enabled;
    query_engine.set_resolver_options(state.resolver_options);
}

void REPL::run() {
    if (repl_utils::is_interactive_terminal()) {
        run_interactive();
    } else {
        // Non-interactive mode - read from stdin
        std::string line;
        while (std::getline(std::cin, line)) {
            execute_command(line);
        }
    }
}

void REPL::run_interactive() {
    print_welcome();
    running = true;
    
    repl_utils::InputHistory history;
    
    while (running) {
        print_prompt();
        
        std::string input = read_multiline_input();
        
        if (input.empty()) continue;
        
        history.add(input);
        
        auto parsed_input = REPLInputParser::parse(input);
        
        if (!parsed_input.valid) {
            print_error(parsed_input.error_message);
            continue;
        }
        
        try {
            switch (parsed_input.command) {
                case REPLCommand::QUERY:
                    handle_query(parsed_input.content);
                    break;
                    
                case REPLCommand::FACT:
                    handle_fact(parsed_input.content);
                    break;
                    
                case REPLCommand::RULE:
                    handle_rule(parsed_input.content);
                    break;
                    
                case REPLCommand::TYPE:
                    handle_type(parsed_input.content);
                    break;
                    
                case REPLCommand::HELP:
                    handle_help();
                    break;
                    
                case REPLCommand::LOAD:
                    handle_load(parsed_input.content);
                    break;
                    
                case REPLCommand::SAVE:
                    handle_save(parsed_input.content);
                    break;
                    
                case REPLCommand::CLEAR:
                    handle_clear();
                    break;
                    
                case REPLCommand::TRACE:
                    handle_trace(parsed_input.content);
                    break;
                    
                case REPLCommand::STATS:
                    handle_stats();
                    break;
                    
                case REPLCommand::QUIT:
                    running = false;
                    std::cout << "Goodbye!\n";
                    break;
                    
                default:
                    print_error("Unknown command");
                    break;
            }
        } catch (const std::exception& e) {
            print_error(repl_utils::format_error(e));
        }
    }
}

void REPL::handle_query(const std::string& query) {
    state.last_query = query;
    state.query_count++;
    
    auto solutions = query_engine.query(query);
    print_solutions(solutions);
}

void REPL::handle_fact(const std::string& fact_str) {
    auto parse_result = Parser::parse_source(fact_str);
    
    if (!parse_result) {
        print_error("Parse error in fact");
        return;
    }
    
    auto& program = *parse_result.value();
    
    for (auto& clause : program.clauses) {
        if (auto fact = dynamic_cast<ast::Fact*>(clause.get())) {
            // Release ownership from program
            clause.release();
            auto fact_ptr = std::unique_ptr<ast::Fact>(fact);
            query_engine.add_fact(std::move(fact_ptr));
            print_info("Fact added successfully");
        }
    }
}

void REPL::handle_rule(const std::string& rule_str) {
    auto parse_result = Parser::parse_source(rule_str);
    
    if (!parse_result) {
        print_error("Parse error in rule");
        return;
    }
    
    auto& program = *parse_result.value();
    
    for (auto& clause : program.clauses) {
        if (auto rule = dynamic_cast<ast::Rule*>(clause.get())) {
            clause.release();
            auto rule_ptr = std::unique_ptr<ast::Rule>(rule);
            query_engine.add_rule(std::move(rule_ptr));
            print_info("Rule added successfully");
        }
    }
}

void REPL::handle_type(const std::string& type_str) {
    auto parse_result = Parser::parse_source(type_str);
    
    if (!parse_result) {
        print_error("Parse error in type definition");
        return;
    }
    
    print_info("Type definition processed");
}

void REPL::handle_help() {
    std::cout << repl_utils::colorize("LogicPP REPL Help", repl_utils::Color::CYAN) << "\n\n";
    
    std::cout << "Commands:\n";
    std::cout << "  ?- goal.                Query a goal\n";
    std::cout << "  fact predicate(args).   Add a fact\n";
    std::cout << "  rule head :- body.      Add a rule\n";
    std::cout << "  type Name = Type.       Define a type\n";
    std::cout << "\n";
    
    std::cout << "REPL Commands:\n";
    std::cout << "  :help                   Show this help\n";
    std::cout << "  :quit, :q               Exit the REPL\n";
    std::cout << "  :load <file>            Load program from file\n";
    std::cout << "  :save <file>            Save knowledge base to file\n";
    std::cout << "  :clear                  Clear knowledge base\n";
    std::cout << "  :trace on|off           Enable/disable tracing\n";
    std::cout << "  :stats                  Show statistics\n";
    std::cout << "\n";
    
    std::cout << "Example:\n";
    std::cout << "  fact parent(john, mary).\n";
    std::cout << "  rule ancestor(X, Y) :- parent(X, Y).\n";
    std::cout << "  ?- ancestor(john, Who).\n\n";
}

void REPL::handle_load(const std::string& filename) {
    if (load_program_file(filename)) {
        print_info(std::format("Loaded program from {}", filename));
    } else {
        print_error(std::format("Failed to load {}", filename));
    }
}

void REPL::handle_save(const std::string& filename) {
    if (save_knowledge_base(filename)) {
        print_info(std::format("Saved knowledge base to {}", filename));
    } else {
        print_error(std::format("Failed to save to {}", filename));
    }
}

void REPL::handle_clear() {
    query_engine.clear();
    state.query_count = 0;
    print_info("Knowledge base cleared");
}

void REPL::handle_trace(const std::string& args) {
    if (args == "on" || args == "true") {
        enable_tracing(true);
        print_info("Tracing enabled");
    } else if (args == "off" || args == "false") {
        enable_tracing(false);
        print_info("Tracing disabled");
    } else {
        print_error("Usage: :trace on|off");
    }
}

void REPL::handle_stats() {
    auto stats = query_engine.get_stats();
    
    std::cout << repl_utils::colorize("Statistics:", repl_utils::Color::CYAN) << "\n";
    std::cout << std::format("  Facts: {}\n", stats.facts);
    std::cout << std::format("  Rules: {}\n", stats.rules);
    std::cout << std::format("  Total clauses: {}\n", stats.total_clauses());
    std::cout << std::format("  Queries executed: {}\n", state.query_count);
    std::cout << "\n";
}

void REPL::print_welcome() {
    std::cout << repl_utils::colorize("Welcome to LogicPP v1.0", repl_utils::Color::GREEN) << "\n";
    std::cout << "A modern logic programming language with types and pattern matching.\n";
    std::cout << "Type :help for help, :quit to exit.\n\n";
}

void REPL::print_prompt() {
    std::cout << repl_utils::colorize("?- ", repl_utils::Color::BLUE);
}

void REPL::print_solutions(const std::vector<Solution>& solutions) {
    if (solutions.empty()) {
        std::cout << repl_utils::colorize("false", repl_utils::Color::RED) << ".\n";
    } else {
        for (size_t i = 0; i < solutions.size(); ++i) {
            if (solutions.size() > 1) {
                std::cout << repl_utils::colorize(
                    std::format("Solution {}:", i + 1), 
                    repl_utils::Color::CYAN) << " ";
            }
            
            std::string solution_str = solutions[i].to_string();
            std::cout << repl_utils::colorize(solution_str, repl_utils::Color::GREEN) << std::endl;
        }
    }
    std::cout << std::endl;
}

void REPL::print_error(const std::string& message) {
    std::cout << repl_utils::colorize("Error: ", repl_utils::Color::RED) << message << "\n\n";
}

void REPL::print_info(const std::string& message) {
    std::cout << repl_utils::colorize(message, repl_utils::Color::GREEN) << "\n\n";
}

std::string REPL::read_multiline_input() {
    std::string input;
    std::string line;
    
    while (std::getline(std::cin, line)) {
        input += line;
        
        if (is_complete_statement(input)) {
            break;
        }
        
        input += "\n";
        std::cout << "   "; // Continuation prompt
    }
    
    return input;
}

bool REPL::is_complete_statement(const std::string& input) {
    // Simple heuristic: statement is complete if it ends with '.' and brackets are balanced
    if (!input.ends_with(".")) {
        return false;
    }
    
    int paren_count = 0;
    int brace_count = 0;
    int bracket_count = 0;
    
    for (char ch : input) {
        switch (ch) {
            case '(': paren_count++; break;
            case ')': paren_count--; break;
            case '{': brace_count++; break;
            case '}': brace_count--; break;
            case '[': bracket_count++; break;
            case ']': bracket_count--; break;
        }
    }
    
    return paren_count == 0 && brace_count == 0 && bracket_count == 0;
}

bool REPL::load_program_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    auto parse_result = Parser::parse_source(content);
    if (!parse_result) {
        return false;
    }
    
    query_engine.load_program(**parse_result);
    return true;
}

bool REPL::save_knowledge_base(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // This would need to serialize the knowledge base
    // For now, just write a comment
    file << "% LogicPP Knowledge Base\n";
    file << "% Generated by REPL\n\n";
    
    return true;
}

void REPL::enable_tracing(bool enabled) {
    state.tracing_enabled = enabled;
    state.resolver_options.trace_execution = enabled;
    query_engine.set_resolver_options(state.resolver_options);
}

// REPL utilities implementation
namespace repl_utils {

std::string format_solutions(const std::vector<Solution>& solutions) {
    if (solutions.empty()) {
        return "false";
    }
    
    std::ostringstream oss;
    for (size_t i = 0; i < solutions.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << solutions[i].to_string();
    }
    
    return oss.str();
}

std::string format_error(const std::exception& e, const std::string& context) {
    if (context.empty()) {
        return e.what();
    }
    return std::format("{}: {}", context, e.what());
}

bool is_interactive_terminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

size_t get_terminal_width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return 80; // Default
#endif
}

std::string colorize(const std::string& text, Color color) {
    if (!is_interactive_terminal()) {
        return text; // No colors in non-interactive mode
    }
    
    const char* color_code = "";
    
    switch (color) {
        case Color::RESET:   color_code = "\033[0m"; break;
        case Color::RED:     color_code = "\033[31m"; break;
        case Color::GREEN:   color_code = "\033[32m"; break;
        case Color::YELLOW:  color_code = "\033[33m"; break;
        case Color::BLUE:    color_code = "\033[34m"; break;
        case Color::MAGENTA: color_code = "\033[35m"; break;
        case Color::CYAN:    color_code = "\033[36m"; break;
    }
    
    return std::format("{}{}\033[0m", color_code, text);
}

// InputHistory implementation
void InputHistory::add(const std::string& input) {
    if (input.empty() || (history.size() > 0 && history.back() == input)) {
        return; // Don't add empty or duplicate entries
    }
    
    history.push_back(input);
    
    if (history.size() > max_size) {
        history.erase(history.begin());
    }
    
    reset_position();
}

std::string InputHistory::get_previous() {
    if (history.empty()) return "";
    
    if (current_position > 0) {
        current_position--;
    }
    
    return history[current_position];
}

std::string InputHistory::get_next() {
    if (history.empty()) return "";
    
    if (current_position < history.size() - 1) {
        current_position++;
        return history[current_position];
    }
    
    reset_position();
    return "";
}

void InputHistory::reset_position() {
    current_position = history.size();
}

} // namespace repl_utils

// REPLApplication implementation
int REPLApplication::run() {
    if (options.trace) {
        repl.enable_tracing(true);
    }
    
    repl.set_resolver_options(options.resolver_options);
    
    if (!options.input_file.empty()) {
        repl.execute_file(options.input_file);
    } else {
        repl.run();
    }
    
    return 0;
}

REPLOptions REPLOptions::parse_args(int argc, char* argv[]) {
    REPLOptions options;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            options.print_help();
            std::exit(0);
        } else if (arg == "--trace") {
            options.trace = true;
        } else if (arg == "--no-typecheck") {
            options.type_check = false;
        } else if (arg == "--file" || arg == "-f") {
            if (i + 1 < argc) {
                options.input_file = argv[++i];
                options.interactive = false;
            }
        }
    }
    
    return options;
}

void REPLOptions::print_help() {
    std::cout << "LogicPP - Modern Logic Programming Language\n\n";
    std::cout << "Usage: logicpp [options] [file]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help          Show this help message\n";
    std::cout << "  -f, --file FILE     Execute program from file\n";
    std::cout << "  --trace             Enable execution tracing\n";
    std::cout << "  --no-typecheck      Disable type checking\n";
    std::cout << "\n";
}

int REPLApplication::main(int argc, char* argv[]) {
    try {
        auto options = REPLOptions::parse_args(argc, argv);
        REPLApplication app(options);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

} // namespace logicpp