# Prolog Examples

This directory contains example Prolog programs and C++ applications demonstrating the CppLProlog interpreter.

For architectural details and data flow diagrams, see:
- [System Architecture](../docs/ARCHITECTURE.md)
- [Data Flow Documentation](../docs/DATA_FLOW.md)

## Structure

- **Prolog Files (*.pl)**: Pure Prolog programs that can be loaded by the interpreter
- **C++ Applications**: Demonstrate how to use the interpreter programmatically

## Prolog Programs

### basic.pl
Simple facts and rules demonstrating basic Prolog concepts:
- Facts about likes relationships
- Rules for happiness and friendship

### family.pl
Comprehensive family tree with relationships:
- Parent/child facts
- Gender classifications
- Derived relationships (father, mother, grandparent, etc.)
- Complex queries about family connections

### lists.pl
List processing predicates:
- Membership testing
- List concatenation (append)
- Length calculation
- Reversal operations
- Element removal
- Sorting checks

## C++ Examples

### basic_example.cpp
Demonstrates basic interpreter usage with simple queries.

### family_tree.cpp
Shows complex relationship queries and family tree visualization.

### list_processing.cpp
Illustrates list manipulation and processing operations.

## Running Examples

```bash
# Build all examples
cmake --build build

# Run individual examples
./build/examples/basic_example
./build/examples/family_tree
./build/examples/list_processing
```

## Loading Prolog Files

Examples show two ways to load Prolog programs:

1. **From file**: `interpreter.loadFile("examples/basic.pl")`
2. **From string**: `interpreter.loadString(program_text)`

The file-based approach is recommended for larger programs as it provides better separation of concerns and easier maintenance.