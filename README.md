# CppLProlog

A modern, high-performance Prolog interpreter implemented in C++23 with comprehensive testing, examples, and documentation.

## Features

- **Modern C++23**: Leverages the latest C++ features for clean, efficient code
- **Complete Prolog Implementation**: Full support for facts, rules, queries, and unification
- **High Performance**: Optimized resolution engine with memory pooling
- **Extensive Testing**: Comprehensive test suite using Google Test
- **Rich Examples**: Multiple example programs demonstrating various Prolog concepts
- **Built-in Predicates**: Support for arithmetic, list operations, and type checking
- **Interactive Mode**: REPL interface for interactive Prolog programming
- **CMake Build System**: Modern build configuration with dependency management

## Quick Start

### Prerequisites

- C++23 compatible compiler (GCC 12+, Clang 15+, MSVC 19.30+)
- CMake 3.25 or higher
- Git

### Building

```bash
git clone <repository-url>
cd CppLProlog
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Running

```bash
# Interactive mode
./src/prolog_interpreter

# Load a Prolog file
./src/prolog_interpreter family_tree.pl

# Execute a query and exit
./src/prolog_interpreter --query "parent(tom, X)" family_tree.pl

# Run tests
./tests/prolog_tests

# Run examples
./examples/basic_example
./examples/family_tree
./examples/list_processing
./examples/arithmetic

# Run benchmarks
./benchmarks/prolog_benchmarks
```

## Architecture

The interpreter consists of several key components:

### Core Components

- **Term System**: Flexible term representation supporting atoms, variables, compounds, integers, floats, strings, and lists
- **Parser**: Recursive descent parser with comprehensive tokenization
- **Unification Engine**: Robinson's unification algorithm with occurs check
- **Resolution Engine**: SLD resolution with backtracking and choice points
- **Database**: Indexed clause storage with efficient retrieval
- **Built-in Predicates**: Extensive library of built-in predicates

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

### Arithmetic

```prolog
% Factorial
factorial(0, 1).
factorial(N, F) :- 
    N > 0,
    N1 is N - 1,
    factorial(N1, F1),
    F is N * F1.

% Query
?- factorial(5, X).
X = 120
```

## Built-in Predicates

### Arithmetic
- `is/2` - Arithmetic evaluation
- `+/3`, `-/3`, `*/3`, `//3` - Basic arithmetic operations
- `=/2` - Unification
- `</2`, `>/2` - Comparison operators

### List Operations
- `append/3` - List concatenation
- `member/2` - List membership
- `length/2` - List length

### Type Checking
- `var/1` - Check if term is variable
- `nonvar/1` - Check if term is not variable
- `atom/1` - Check if term is atom
- `number/1` - Check if term is number

### Control
- `true/0` - Always succeeds
- `fail/0` - Always fails
- `!/0` - Cut operator (planned)

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

Performance benchmarks are included to measure:

- Parsing speed
- Unification performance
- Resolution efficiency
- Memory usage

```bash
./benchmarks/prolog_benchmarks
```

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

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

### Code Style

- Follow modern C++23 practices
- Use descriptive variable names
- Include comprehensive tests
- Document public APIs
- Follow existing naming conventions

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Built with modern C++23 features
- Uses Google Test for testing
- Uses Google Benchmark for performance measurement
- Inspired by classic Prolog implementations like SWI-Prolog and GNU Prolog