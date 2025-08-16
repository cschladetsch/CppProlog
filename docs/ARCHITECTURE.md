# CppLProlog Architecture

This document describes the internal architecture and design decisions of the CppLProlog interpreter.

## Overview

CppLProlog is designed as a modular, high-performance Prolog interpreter using modern C++23 features. The architecture follows clean separation of concerns with well-defined interfaces between components.

## Core Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Interpreter   │───▶│    Database     │───▶│    Resolver     │
│                 │    │                 │    │                 │
│  - REPL         │    │  - Clause Store │    │  - SLD Resolve  │
│  - Query Exec   │    │  - Indexing     │    │  - Backtracking │
│  - File Loading │    │  - Retrieval    │    │  - Choice Points│
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │              ┌─────────────────┐             │
         └─────────────▶│     Parser      │◀────────────┘
                        │                 │
                        │  - Lexer        │
                        │  - AST Builder  │
                        │  - Error Handle │
                        └─────────────────┘
                                 │
                        ┌─────────────────┐
                        │   Term System   │
                        │                 │
                        │  - Term Types   │
                        │  - Unification  │
                        │  - Substitution │
                        └─────────────────┘
```

## Component Details

### Term System (`src/prolog/term.h/cpp`)

The term system forms the foundation of the interpreter, representing all Prolog data structures.

#### Term Hierarchy

```cpp
Term (abstract base)
├── Atom
├── Variable  
├── Integer
├── Float
├── String
├── Compound
└── List
```

#### Key Design Decisions

1. **Shared Pointer Management**: Terms use `std::shared_ptr` for automatic memory management
2. **Type Safety**: Runtime type checking with `is<T>()` and `as<T>()` methods
3. **Immutability**: Terms are immutable after creation (clone for modifications)
4. **Hash Support**: All terms implement consistent hashing for indexing

#### Memory Management

```cpp
using TermPtr = std::shared_ptr<Term>;
using TermList = std::vector<TermPtr>;

// Factory functions ensure consistent creation
TermPtr makeAtom(const std::string& name);
TermPtr makeCompound(const std::string& functor, TermList arguments);
```

### Unification Engine (`src/prolog/unification.h/cpp`)

Implements Robinson's unification algorithm with occurs check.

#### Algorithm

1. **Dereference**: Follow variable bindings in substitution
2. **Variable Handling**: Unify variables with terms or other variables
3. **Structure Unification**: Recursively unify compound terms
4. **Occurs Check**: Prevent infinite structures (X = f(X))

#### Substitution Management

```cpp
using Substitution = std::unordered_map<std::string, TermPtr>;

class Unification {
public:
    static std::optional<Substitution> unify(const TermPtr& t1, const TermPtr& t2);
    static TermPtr applySubstitution(const TermPtr& term, const Substitution& subst);
    static Substitution compose(const Substitution& s1, const Substitution& s2);
};
```

### Parser (`src/prolog/parser.h/cpp`)

Two-phase parsing: lexical analysis followed by syntax analysis.

#### Lexer

- **Token Types**: Atoms, variables, numbers, strings, operators, punctuation
- **Comment Handling**: Skip % comments
- **String Escaping**: Support for escape sequences
- **Error Recovery**: Position tracking for error reporting

#### Parser

- **Recursive Descent**: Clean, maintainable parsing strategy
- **Operator Precedence**: Proper handling of Prolog operators
- **Error Reporting**: Detailed syntax error messages

#### Grammar Support

```prolog
Clause    := Term '.' | Term ':-' TermList '.'
Term      := Atom | Variable | Number | String | Compound | List
Compound  := Atom '(' TermList ')'
List      := '[' TermList ']' | '[' TermList '|' Term ']'
TermList  := Term (',' Term)*
```

### Database (`src/prolog/database.h/cpp`)

Efficient storage and retrieval of Prolog clauses.

#### Indexing Strategy

- **Functor/Arity Index**: Primary index on predicate functor and arity
- **First Argument Index**: Secondary index for goal-directed search (future)
- **Hash-based Lookup**: O(1) average case retrieval

#### Storage Model

```cpp
class Database {
private:
    std::vector<ClausePtr> clauses_;                    // Sequential storage
    std::unordered_map<std::string, std::vector<size_t>> index_;  // Functor/arity index
    
public:
    std::vector<ClausePtr> findMatchingClauses(const TermPtr& goal);
};
```

### Resolution Engine (`src/prolog/resolver.h/cpp`)

Implements SLD resolution with chronological backtracking.

#### Resolution Strategy

1. **Goal Selection**: Left-to-right goal ordering
2. **Clause Selection**: Database order with backtracking
3. **Unification**: Attempt unification with each matching clause
4. **Substitution**: Apply substitutions to remaining goals
5. **Recursion**: Resolve new goal set

#### Choice Point Management

```cpp
class Choice {
    TermPtr goal;                    // Current goal
    TermList remaining_goals;        // Goals to resolve after current
    std::vector<ClausePtr> clauses;  // Available clauses for current goal
    size_t clause_index;             // Next clause to try
    Substitution bindings;           // Current variable bindings
};
```

#### Backtracking Algorithm

```cpp
bool Resolver::solveGoals(const TermList& goals, const Substitution& bindings) {
    if (goals.empty()) {
        // Success - all goals resolved
        return callback(Solution{bindings});
    }
    
    // Try to resolve first goal
    auto matching_clauses = database_.findMatchingClauses(goals[0]);
    pushChoice(goals[0], remaining_goals, matching_clauses, bindings);
    
    return backtrack();
}
```

### Built-in Predicates (`src/prolog/builtin_predicates.h/cpp`)

Extensible system for built-in predicates.

#### Registration System

```cpp
using BuiltinHandler = std::function<bool(const TermList&, Substitution&, 
                                         std::function<bool(const Solution&)>)>;

static std::unordered_map<std::string, BuiltinHandler> builtins_;

// Register new built-in
builtins_["functor/arity"] = handler_function;
```

#### Arithmetic Integration

- **Expression Evaluation**: `is/2` predicate evaluates arithmetic expressions
- **Comparison Operators**: `</2`, `>/2`, `=</2`, `>=/2`, `=:=/2`, `=\\=/2`
- **Type Coercion**: Automatic integer/float conversion

### Interpreter (`src/prolog/interpreter.h/cpp`)

High-level interface combining all components.

#### Query Processing Pipeline

```
User Input → Parser → Query Term → Resolver → Solutions → Output Formatter
```

#### Interactive Mode

- **REPL Loop**: Read-eval-print loop with command processing
- **Command System**: Built-in commands for database inspection
- **Error Handling**: Graceful error recovery and reporting

## Memory Management

### Strategy

1. **Smart Pointers**: `std::shared_ptr` for terms, `std::unique_ptr` for clauses
2. **Copy-on-Write**: Terms are immutable, clone when modification needed  
3. **Memory Pools**: Optional memory pooling for high-frequency allocations
4. **RAII**: All resources managed by constructors/destructors

### Performance Considerations

- **Reference Counting**: Overhead of shared_ptr managed through careful design
- **Memory Locality**: Vector-based storage for cache efficiency
- **Minimal Copying**: Pass by reference, clone only when necessary

## Error Handling

### Exception Hierarchy

```cpp
std::exception
├── ParseException      // Syntax errors
├── UnificationError   // Unification failures  
└── RuntimeError       // General runtime errors
```

### Error Recovery

- **Parser**: Skip to next clause boundary on syntax error
- **Runtime**: Graceful handling of built-in predicate failures
- **User Errors**: Informative error messages with context

## Thread Safety

### Current Status

- **Single-threaded**: Current implementation is not thread-safe
- **Future Plans**: Thread-local databases, immutable terms enable concurrency

### Concurrency Opportunities

1. **Parallel Resolution**: Multiple choice points can be explored concurrently
2. **Concurrent Queries**: Independent queries can run in parallel
3. **Lock-free Data Structures**: Immutable terms enable lock-free sharing

## Extension Points

### Adding New Term Types

1. Inherit from `Term` base class
2. Implement required virtual methods
3. Add factory function
4. Update parser and unification engine

### Adding Built-in Predicates

```cpp
bool myPredicate(const TermList& args, Substitution& bindings, 
                 std::function<bool(const Solution&)> callback) {
    // Implementation
    return success;
}

// Register during initialization
BuiltinPredicates::register("my_predicate/2", myPredicate);
```

### Custom Indexing Strategies

The database interface allows for custom indexing implementations:

```cpp
class CustomDatabase : public Database {
    // Override indexing methods
    std::vector<ClausePtr> findMatchingClauses(const TermPtr& goal) override;
};
```

## Testing Architecture

### Test Structure

```
tests/
├── test_term.cpp           # Term system tests
├── test_unification.cpp    # Unification algorithm tests  
├── test_parser.cpp         # Parser and lexer tests
├── test_database.cpp       # Database storage tests
├── test_resolver.cpp       # Resolution engine tests
├── test_interpreter.cpp    # Integration tests
└── test_builtin_predicates.cpp # Built-in predicate tests
```

### Testing Strategy

1. **Unit Tests**: Each component tested in isolation
2. **Integration Tests**: End-to-end functionality
3. **Property-based Tests**: Invariant checking (planned)
4. **Performance Tests**: Benchmark critical paths

## Benchmarking

### Metrics

- **Parsing Speed**: Clauses parsed per second
- **Unification Rate**: Unifications per second  
- **Resolution Throughput**: Goals resolved per second
- **Memory Usage**: Peak memory consumption

### Benchmark Structure

```cpp
BENCHMARK(BM_ParseSimpleClause);
BENCHMARK(BM_UnifyCompoundTerms);
BENCHMARK(BM_ResolveFactQuery);
BENCHMARK(BM_ResolveRuleQuery);
```

## Future Enhancements

### Short Term

1. **Cut Operator**: Implement Prolog cut (!) for deterministic predicates
2. **More Built-ins**: Expand built-in predicate library
3. **Debugging Support**: Add trace and debug modes
4. **Module System**: Namespace support for large programs

### Long Term

1. **Constraint Logic Programming**: CLP(FD), CLP(R) extensions
2. **Tabling/Memoization**: Cache intermediate results
3. **Parallel Resolution**: Multi-threaded goal resolution
4. **JIT Compilation**: Compile frequently used predicates

### Performance Optimizations

1. **First Argument Indexing**: Index on first argument of goals
2. **Clause Indexing**: More sophisticated indexing strategies  
3. **Memory Pool Optimization**: Reduce allocation overhead
4. **SIMD Unification**: Vectorized unification for certain patterns