# CppLProlog Data Flow Documentation

This document illustrates the data flow through the CppLProlog interpreter using Mermaid diagrams.

## Query Execution Flow

### Complete Query Processing

```mermaid
sequenceDiagram
    participant User
    participant Interpreter
    participant Parser
    participant Database
    participant Resolver
    participant Unification
    participant Output
    
    User->>Interpreter: query("likes(X, wine)")
    Interpreter->>Parser: parseQuery("likes(X, wine)")
    Parser->>Parser: tokenize()
    Parser->>Parser: buildAST()
    Parser-->>Interpreter: QueryTerm
    
    Interpreter->>Database: findMatchingClauses(QueryTerm)
    Database-->>Interpreter: [Clause1, Clause2, ...]
    
    Interpreter->>Resolver: solve(QueryTerm, Clauses)
    
    loop For each clause
        Resolver->>Unification: unify(QueryTerm, ClauseHead)
        Unification-->>Resolver: Substitution | null
        
        alt Unification succeeds
            Resolver->>Resolver: applySubstitution(ClauseBody)
            Resolver->>Resolver: recursiveSolve(NewGoals)
        else Unification fails
            Resolver->>Resolver: backtrack()
        end
    end
    
    Resolver-->>Interpreter: Solutions[]
    Interpreter->>Output: formatSolutions(Solutions)
    Output-->>User: Display results
```

## Term Creation and Management

### Term Factory Pattern

```mermaid
flowchart TD
    A[User Code] --> B[makeAtom]
    A --> C[makeVariable]
    A --> D[makeCompound]
    A --> E[makeList]
    
    B --> F[Atom Constructor]
    C --> G[Variable Constructor]
    D --> H[Compound Constructor]
    E --> I[List Constructor]
    
    F --> J[shared_ptr&lt;Atom&gt;]
    G --> K[shared_ptr&lt;Variable&gt;]
    H --> L[shared_ptr&lt;Compound&gt;]
    I --> M[shared_ptr&lt;List&gt;]
    
    J --> N[Reference Counting]
    K --> N
    L --> N
    M --> N
    
    N --> O[Automatic Cleanup]
    
    style J fill:#e3f2fd
    style K fill:#e8f5e8
    style L fill:#fff3e0
    style M fill:#fce4ec
    style O fill:#c8e6c9
```

## Database Indexing System

### Clause Storage and Retrieval

```mermaid
graph TD
    A[New Clause] --> B[Extract Functor/Arity]
    B --> C[Generate Index Key]
    C --> D[functor/arity]
    D --> E[Index Map Lookup]
    
    E --> F{Key Exists?}
    F -->|No| G[Create New Entry]
    F -->|Yes| H[Append to Existing]
    
    G --> I[Vector&lt;ClausePtr&gt;]
    H --> I
    I --> J[Store in Database]
    
    K[Query Goal] --> L[Extract Functor/Arity]
    L --> M[Index Lookup]
    M --> N[Retrieve Matching Clauses]
    N --> O[Return Vector&lt;ClausePtr&gt;]
    
    style A fill:#e3f2fd
    style K fill:#fff3e0
    style J fill:#c8e6c9
    style O fill:#e8f5e8
```

## Unification Process Detail

### Step-by-Step Unification

```mermaid
stateDiagram-v2
    [*] --> Dereference
    
    Dereference --> SameTermCheck
    SameTermCheck --> Success : Terms identical
    SameTermCheck --> VariableCheck : Terms different
    
    VariableCheck --> VarVar : Both variables
    VariableCheck --> VarTerm : One variable
    VariableCheck --> TermTerm : No variables
    
    VarVar --> Success : Create binding
    
    VarTerm --> OccursCheck
    OccursCheck --> Failure : Variable occurs in term
    OccursCheck --> Success : Safe binding
    
    TermTerm --> TypeCheck
    TypeCheck --> Failure : Different types
    TypeCheck --> StructureCheck : Same type
    
    StructureCheck --> Failure : Different structure
    StructureCheck --> RecursiveUnify : Same structure
    
    RecursiveUnify --> Success : All arguments unify
    RecursiveUnify --> Failure : Some arguments fail
    
    Success --> [*]
    Failure --> [*]
```

## Resolution Tree Exploration

### Backtracking Search Space

```mermaid
graph TD
    A[Goal: likes(X, wine)] --> B[Clause 1: likes(mary, wine)]
    A --> C[Clause 2: likes(john, wine)]
    A --> D[Clause 3: likes(mary, food)]
    
    B --> E[Unify: X = mary]
    C --> F[Unify: X = john]
    D --> G[Unify: X = mary, wine ≠ food]
    
    E --> H[Success: X = mary]
    F --> I[Success: X = john]
    G --> J[Failure: Backtrack]
    
    K[Goal: happy(X)] --> L[Clause: happy(Y) :- likes(Y, wine)]
    L --> M[Unify: X = Y]
    M --> N[New Goal: likes(Y, wine)]
    N --> O[Recursive Resolution]
    O --> P[Success: X = mary, X = john]
    
    style H fill:#c8e6c9
    style I fill:#c8e6c9
    style P fill:#c8e6c9
    style J fill:#ffcdd2
```

## Memory Ownership Model

### Reference Management

```mermaid
graph TD
    A[Parser] --> B[Creates Terms]
    B --> C[shared_ptr&lt;Term&gt;]
    
    D[Database] --> E[Stores Clauses]
    E --> F[unique_ptr&lt;Clause&gt;]
    F --> G[Contains TermPtrs]
    G --> C
    
    H[Resolver] --> I[Creates Substitutions]
    I --> J[map&lt;string, TermPtr&gt;]
    J --> C
    
    K[Solution] --> L[Contains Substitution]
    L --> J
    
    C --> M[Reference Count]
    M --> N{Count = 0?}
    N -->|Yes| O[Automatic Deletion]
    N -->|No| P[Keep Alive]
    
    Q[Stack Unwinding] --> R[unique_ptr Cleanup]
    R --> S[RAII Deletion]
    
    style O fill:#ffcdd2
    style S fill:#ffcdd2
    style P fill:#c8e6c9
    style C fill:#e3f2fd
```

## Error Propagation

### Exception Handling Flow

```mermaid
flowchart TD
    A[User Input] --> B[Parser]
    B --> C{Parse Error?}
    C -->|Yes| D[ParseException]
    C -->|No| E[Resolver]
    
    E --> F[Unification]
    F --> G{Unify Error?}
    G -->|Yes| H[UnificationError]
    G -->|No| I[Built-in Predicates]
    
    I --> J{Runtime Error?}
    J -->|Yes| K[RuntimeError]
    J -->|No| L[Success]
    
    D --> M[Error Handler]
    H --> M
    K --> M
    
    M --> N[Error Message]
    N --> O[User Display]
    
    L --> P[Format Results]
    P --> O
    
    style D fill:#ffcdd2
    style H fill:#ffcdd2
    style K fill:#ffcdd2
    style L fill:#c8e6c9
    style M fill:#fff3e0
```

## Performance Optimization Points

### Bottleneck Analysis

```mermaid
graph TD
    A[Query Input] --> B[Parsing Cost]
    B --> C[Term Creation]
    C --> D[Database Lookup]
    D --> E[Unification Cost]
    E --> F[Substitution Application]
    F --> G[Recursive Resolution]
    G --> H[Backtracking Overhead]
    H --> I[Solution Formatting]
    
    B --> B1[Optimization: Incremental Parsing]
    C --> C1[Optimization: Memory Pools]
    D --> D1[Optimization: Better Indexing]
    E --> E1[Optimization: Fast Path Unification]
    F --> F1[Optimization: In-place Updates]
    G --> G1[Optimization: Tail Call Optimization]
    H --> H1[Optimization: Choice Point Pruning]
    I --> I1[Optimization: Lazy Formatting]
    
    style B1 fill:#e8f5e8
    style C1 fill:#e8f5e8
    style D1 fill:#e8f5e8
    style E1 fill:#e8f5e8
    style F1 fill:#e8f5e8
    style G1 fill:#e8f5e8
    style H1 fill:#e8f5e8
    style I1 fill:#e8f5e8
```

## Concurrent Access Model (Future)

### Thread Safety Considerations

```mermaid
graph TD
    A[Thread 1] --> B[Read-only Database]
    C[Thread 2] --> B
    D[Thread 3] --> B
    
    A --> E[Local Resolver]
    C --> F[Local Resolver]
    D --> G[Local Resolver]
    
    E --> H[Private Choice Stack]
    F --> I[Private Choice Stack]
    G --> J[Private Choice Stack]
    
    H --> K[Immutable Terms]
    I --> K
    J --> K
    
    L[Master Thread] --> M[Database Updates]
    M --> N[Write Lock]
    N --> B
    
    style K fill:#e3f2fd
    style B fill:#fff3e0
    style N fill:#ffcdd2
```