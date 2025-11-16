#pragma once

#include "resolver.hpp"
#include "type_system.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>

namespace logicpp {

// REPL command types
enum class REPLCommand {
    QUERY,          // ?- goal.
    FACT,           // fact declaration
    RULE,           // rule declaration
    TYPE,           // type definition
    FUNCTION,       // function definition
    HELP,           // :help
    QUIT,           // :quit
    LOAD,           // :load filename
    SAVE,           // :save filename
    CLEAR,          // :clear
    TRACE,          // :trace on/off
    STATS,          // :stats
    UNKNOWN
};

// REPL command parser
struct REPLInput {
    REPLCommand command;
    std::string content;
    bool valid;
    std::string error_message;
    
    REPLInput(REPLCommand cmd, std::string cont) 
        : command(cmd), content(std::move(cont)), valid(true) {}
    
    REPLInput(std::string error) 
        : command(REPLCommand::UNKNOWN), valid(false), error_message(std::move(error)) {}
};

class REPLInputParser {
public:
    static REPLInput parse(const std::string& input);
    
private:
    static REPLCommand classify_input(const std::string& input);
    static std::string trim(const std::string& str);
};

// REPL session state
struct REPLState {
    bool tracing_enabled = false;
    ResolverOptions resolver_options;
    std::string last_query;
    size_t query_count = 0;
    
    REPLState() {
        resolver_options = ResolverOptions::default_options();
    }
};

// Main REPL interface
class REPL {
private:
    QueryEngine query_engine;
    TypeChecker type_checker;
    REPLState state;
    bool running = false;
    
    // Command handlers
    void handle_query(const std::string& query);
    void handle_fact(const std::string& fact_str);
    void handle_rule(const std::string& rule_str);
    void handle_type(const std::string& type_str);
    void handle_help();
    void handle_load(const std::string& filename);
    void handle_save(const std::string& filename);
    void handle_clear();
    void handle_trace(const std::string& args);
    void handle_stats();
    
    // Utility methods
    void print_welcome();
    void print_prompt();
    void print_solutions(const std::vector<Solution>& solutions);
    void print_error(const std::string& message);
    void print_info(const std::string& message);
    
    // Input handling
    std::string read_multiline_input();
    bool is_complete_statement(const std::string& input);
    
    // File I/O
    bool load_program_file(const std::string& filename);
    bool save_knowledge_base(const std::string& filename);

public:
    REPL();
    
    // Main REPL loop
    void run();
    void run_interactive();
    
    // Non-interactive mode
    void execute_command(const std::string& command);
    void execute_file(const std::string& filename);
    
    // Configuration
    void set_resolver_options(const ResolverOptions& options);
    void enable_tracing(bool enabled);
    
    // Batch mode
    std::vector<std::string> execute_batch(const std::vector<std::string>& commands);
};

// REPL utilities
namespace repl_utils {
    
    // Format solutions for display
    std::string format_solutions(const std::vector<Solution>& solutions);
    
    // Format error messages
    std::string format_error(const std::exception& e, const std::string& context = "");
    
    // Check if running in interactive terminal
    bool is_interactive_terminal();
    
    // Get terminal width for formatting
    size_t get_terminal_width();
    
    // Color output support
    enum class Color { RESET, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN };
    std::string colorize(const std::string& text, Color color);
    
    // Input history management
    class InputHistory {
    private:
        std::vector<std::string> history;
        size_t max_size;
        size_t current_position;
        
    public:
        explicit InputHistory(size_t max_sz = 1000) 
            : max_size(max_sz), current_position(0) {}
        
        void add(const std::string& input);
        std::string get_previous();
        std::string get_next();
        void reset_position();
        
        const std::vector<std::string>& get_history() const { return history; }
        void clear() { history.clear(); current_position = 0; }
        
        // Save/load history to file
        bool save_to_file(const std::string& filename) const;
        bool load_from_file(const std::string& filename);
    };
    
} // namespace repl_utils

// Command-line argument parser for the REPL
struct REPLOptions {
    bool interactive = true;
    std::string input_file;
    std::string output_file;
    bool trace = false;
    bool type_check = true;
    ResolverOptions resolver_options;
    
    static REPLOptions parse_args(int argc, char* argv[]);
    void print_help();
};

// Main entry point for REPL application
class REPLApplication {
private:
    REPLOptions options;
    REPL repl;
    
public:
    explicit REPLApplication(REPLOptions opts) : options(std::move(opts)) {}
    
    int run();
    
    static int main(int argc, char* argv[]);
};

} // namespace logicpp