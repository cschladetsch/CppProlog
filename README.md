# CppLProlog

A modern, high-performance Prolog interpreter implemented in C++23 with comprehensive testing, benchmarking, and rich documentation. This implementation demonstrates the complexity and engineering effort required to recreate Prolog's declarative programming paradigm in C++.

## Features

- **Modern C++23**: Leverages the latest C++ features for clean, efficient code
- **Complete Prolog Implementation**: Full support for facts, rules, queries, unification, and cut operator
- **High Performance**: Optimized resolution engine with memory pooling and release-mode builds
- **Extensive Testing**: Comprehensive test suite using Google Test with 173 tests and complete coverage
- **Performance Benchmarking**: Google Benchmark integration for resolution performance analysis
- **Rich Examples**: Multiple example programs demonstrating various Prolog concepts
- **Built-in Predicates**: Comprehensive set including list operations, arithmetic, type checking, and control structures
- **Interactive REPL**: Full-featured interactive mode with colored output
- **Robust Architecture**: ~2,200 lines of carefully crafted C++ implementing Prolog semantics with performance optimizations
- **CMake Build System**: Modern build configuration with dependency management

## Quick Start

### Prerequisites

- C++23 compatible compiler (GCC 12+, Clang 15+, MSVC 19.30+)
- CMake 3.25 or higher
- Git (with submodule support for rang.hpp)

### Building

```bash
git clone --recursive https://github.com/cschladetsch/CppProlog
cd CppLProlog
mkdir build
cd build

# Debug build (default)
cmake ..
make -j$(nproc)

# Release build (optimized performance)
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

**Note**: The `--recursive` flag is required to clone the rang.hpp submodule for colored terminal output.

**Tip**: Use Release mode for performance benchmarking and production use.

### Running

All executables are now built in the `build/bin/` directory:

```bash
# Interactive mode
./bin/prolog_interpreter

# Load a Prolog file
./bin/prolog_interpreter examples/family.pl

# Execute a query and exit
./bin/prolog_interpreter --query "parent(tom, X)" examples/family.pl

# Run comprehensive test suite (173 tests)
./bin/prolog_tests

# Run example programs
./bin/basic_example
./bin/family_tree
./bin/list_processing
./bin/arithmetic

# Run performance benchmarks (use Release build)
./bin/prolog_benchmarks

# Run interactive demo
../demo.sh
```

## Architecture

The interpreter consists of several key components working together to provide a complete Prolog environment.

### Core Components

- **Term System**: Flexible term representation supporting atoms, variables, compounds, integers, floats, strings, and lists
- **Parser**: Recursive descent parser with comprehensive tokenization
- **Unification Engine**: Robinson's unification algorithm with occurs check
- **Resolution Engine**: SLD resolution with backtracking and choice points
- **Database**: Indexed clause storage with efficient retrieval
- **Built-in Predicates**: Extensive library of built-in predicates

### Documentation

For detailed architecture information, see:

- **[Architecture Overview](docs/ARCHITECTURE.md)** - Complete system architecture with Mermaid diagrams
- **[Data Flow Diagrams](docs/DATA_FLOW.md)** - Visual representation of data flow through components
- **[API Reference](docs/API.md)** - Detailed API documentation

### Key Classes

```cpp
// Term hierarchy
class Term;
class Atom, Variable, Integer, Float, String, Compound, List;

// Core logic
class Unification;
class Resolver;
class Database;
class Interpreter;

// Parsing
class Lexer;
class Parser;
```

## Examples

### Basic Facts and Rules

```prolog
% Facts
parent(tom, bob).
parent(bob, ann).

% Rules
grandparent(X, Z) :- parent(X, Y), parent(Y, Z).

% Query
?- grandparent(tom, ann).
```

### List Processing

```prolog
% List membership
member(X, [X|_]).
member(X, [_|T]) :- member(X, T).

% List append
append([], L, L).
append([H|T], L, [H|R]) :- append(T, L, R).

% Query
?- append([1,2], [3,4], L).
L = [1, 2, 3, 4]
```

### Family Relationships

```prolog
% Extended family tree
parent(tom, bob).
parent(tom, liz).
parent(bob, ann).
parent(liz, pat).

male(tom). male(bob). 
female(liz). female(ann). female(pat).

% Rules for relationships
father(X, Y) :- parent(X, Y), male(X).
mother(X, Y) :- parent(X, Y), female(X).
grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
sibling(X, Y) :- parent(Z, X), parent(Z, Y).

% Query
?- grandparent(tom, ann).
true
```

## Built-in Predicates

The interpreter provides a comprehensive set of built-in predicates for practical Prolog programming:

### Arithmetic Operations
- `is/2` - Arithmetic evaluation with full expression support
  - Supports: `+`, `-`, `*`, `/`, `//` (integer division), `mod`, unary `-`, `abs`
  - Example: `X is 2 + 3 * 4` evaluates to `X = 14`
- `+/3`, `-/3`, `*/3`, `//3` - Direct arithmetic predicates

### Comparison Operators
- `=/2` - Unification operator
- `\=/2` - Negation of unification  
- `</2`, `>/2` - Arithmetic comparison (less than, greater than)
- `=</2`, `>=/2` - Arithmetic comparison (less/equal, greater/equal)
- `==/2`, `\==/2` - Strict term equality/inequality for structural comparison

### List Operations
- `append/3` - List concatenation with backtracking
- `member/2` - List membership testing with backtracking
- `length/2` - Calculate or verify list length with bidirectional support

### Type Checking Predicates
- `var/1` - Test if term is an unbound variable
- `nonvar/1` - Test if term is bound (not a variable)
- `atom/1` - Test if term is an atom
- `number/1` - Test if term is a number (integer or float)
- `integer/1` - Test if term is specifically an integer
- `float/1` - Test if term is specifically a float
- `compound/1` - Test if term is a compound structure
- `ground/1` - Test if term is fully instantiated (no variables)

### Control Structures
- `true/0` - Always succeeds
- `fail/0` - Always fails (forces backtracking)
- `\+/1` - Negation as failure (succeeds if goal fails)
- `!/0` - Cut operator for preventing backtracking

### Input/Output
- `write/1` - Output a term to stdout
- `nl/0` - Output a newline character

### Advanced Features
- **First Argument Indexing**: Database indexing for improved query performance
- **Comprehensive term ordering**: Variables < Numbers < Atoms < Strings < Compounds < Lists
- **Modern C++ implementation**: Uses STL algorithms like `std::lexicographical_compare`
- **Efficient comparison**: Map-based comparator system with lambda functions
- **Docker Support**: Full containerization with multi-stage builds

## Testing

The project includes comprehensive tests covering all major components:

```bash
# Run all tests
make test

# Run specific test categories
./tests/prolog_tests --gtest_filter="TermTest.*"
./tests/prolog_tests --gtest_filter="UnificationTest.*"
./tests/prolog_tests --gtest_filter="ResolverTest.*"
```

## Performance

The implementation includes comprehensive benchmarking to measure:

- **Parsing Speed**: Tokenization and clause parsing performance
- **Unification Performance**: Robinson's algorithm with occurs check
- **Resolution Efficiency**: SLD resolution with backtracking optimization
- **Memory Usage**: Memory pool allocation and term management
- **Query Complexity**: Performance across different query types

```bash
# Run all benchmarks (ensure Release build for accurate results)
./benchmarks/prolog_benchmarks

# Example benchmark results (Release mode):
# BM_ResolveFact              ~100 ns per query
# BM_ResolveSimpleRule        ~500 ns per query  
# BM_ResolveRecursiveRule     ~2000 ns per query
```

**Note**: For accurate performance measurements, always use Release builds (`-DCMAKE_BUILD_TYPE=Release`).

## Interactive Mode

The interpreter supports an interactive REPL mode:

```bash
$ ./prolog_interpreter
CppLProlog Interpreter v1.0
Type :help for commands, or enter Prolog queries.

?- parent(tom, bob).
true.

?- parent(tom, X).
X = bob ;
X = liz.

?- :help
Commands:
  :help, :h     - Show this help
  :quit, :q     - Exit interpreter
  :load <file>  - Load Prolog file
  :clear        - Clear database
  :list         - List all clauses
  :stats        - Show statistics
```

## API Usage

### Programmatic Usage

```cpp
#include "prolog/interpreter.h"

int main() {
    prolog::Interpreter interpreter(false); // Non-interactive
    
    // Load program
    interpreter.loadString(R"(
        parent(tom, bob).
        parent(bob, ann).
        grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
    )");
    
    // Execute query
    auto solutions = interpreter.query("grandparent(tom, ann)");
    
    for (const auto& solution : solutions) {
        std::cout << solution.toString() << std::endl;
    }
    
    return 0;
}
```

### Custom Term Creation

```cpp
#include "prolog/term.h"

// Create terms
auto atom = prolog::makeAtom("hello");
auto var = prolog::makeVariable("X");
auto compound = prolog::makeCompound("func", {atom, var});
auto list = prolog::makeList({atom, var});

// Unify terms
auto result = prolog::Unification::unify(compound, other_term);
if (result) {
    // Use substitution
    auto substituted = prolog::Unification::applySubstitution(term, *result);
}
```

### Code Style

- Follow modern C++23 practices
- Use descriptive variable names
- Include comprehensive tests
- Document public APIs
- Follow existing naming conventions

## Dependencies

### External Libraries

- **[rang.hpp](https://github.com/agauniyal/rang)**: Header-only library for colored terminal output
  - Provides cross-platform terminal color support
  - Included as a git submodule in `External/rang/`
  - No additional installation required

### Build Dependencies

- **Google Test**: Testing framework (automatically fetched by CMake)
- **Google Benchmark**: Performance measurement (automatically fetched by CMake)

## Implementation Insights

This implementation demonstrates the significant engineering effort required to recreate Prolog's declarative programming model in C++:

- **2,200+ lines** of carefully crafted C++ code
- **Complex term hierarchy** with virtual dispatch and smart pointer management
- **Sophisticated unification engine** implementing Robinson's algorithm with occurs check
- **Advanced resolution engine** with choice point management and backtracking
- **Memory management** through custom pooling and RAII principles

### Raw C++ vs Prolog Comparison

What takes ~50 lines in pure Prolog requires over 2,000 lines in C++ to implement the underlying engine. This showcases both the power of Prolog's declarative paradigm and the complexity involved in implementing logical programming systems.

## Acknowledgments

- Built with modern C++23 features and best practices
- Uses Google Test for comprehensive testing coverage
- Uses Google Benchmark for detailed performance analysis
- Uses rang.hpp for enhanced terminal output with colors
- Inspired by classic Prolog implementations like SWI-Prolog and GNU Prolog
- Architecture designed for educational understanding of Prolog internals
