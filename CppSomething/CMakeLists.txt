cmake_minimum_required(VERSION 3.15 FATAL_ERROR) # Minimum CMake version for modern C++ features and target_compile_options syntax.

# Define the project name and the languages used.
project(LogicPP LANGUAGES CXX)

# Set the C++ standard to C++20.
# - std::format requires C++20.
# - std::expected is officially a C++23 feature. However, many compilers (like recent GCC/Clang)
#   provide it with C++20 as an extension or via a newer standard library.
#   If you encounter compilation errors related to std::expected, you might need to
#   change this to C++23: `set(CMAKE_CXX_STANDARD 23)`.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Enforce that the chosen C++ standard must be supported.
set(CMAKE_CXX_EXTENSIONS OFF)        # Prefer standard-compliant C++ over compiler-specific extensions.

# List all source files that make up the LogicPP executable.
set(LOGICPP_SOURCES
    repl.cpp
    parser.cpp
    lexer.cpp
    resolver.cpp
    unification.cpp
    type_system.cpp
)

# Add an executable target named 'logicpp' built from the specified source files.
add_executable(logicpp ${LOGICPP_SOURCES})

# Specify include directories for the 'logicpp' target.
# All project header files (.hpp) are located in the same directory as the source files.
target_include_directories(logicpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Configure compiler options for the 'logicpp' target based on the build type
# (e.g., Debug, Release) and the compiler being used (e.g., GCC, Clang, MSVC).
# This section enables useful warnings and appropriate optimization levels.
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(logicpp PRIVATE
        # Options for GNU (g++) and Clang compilers:
        # -Wall: Enable all standard warnings.
        # -Wextra: Enable extra warnings not covered by -Wall.
        # -Wpedantic: Issue warnings for non-standard C++ constructs.
        # -Wshadow: Warn if a local variable or type shadows another.
        # -Wno-unused-parameter: Suppress warnings for unused function parameters (common in interfaces/abstract methods).
        # -g: Include debugging information.
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wno-unused-parameter;-g>

        # Options for Microsoft Visual C++ (MSVC) compiler:
        # /W4: Set warning level to 4 (most comprehensive).
        # /WX: Treat all compiler warnings as errors (useful for strict code quality).
        # /Zi: Generate complete debugging information.
        $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/Zi>
    )
else() # For Release or other non-Debug build types
    target_compile_options(logicpp PRIVATE
        # Options for GNU and Clang compilers:
        # -O3: Aggressive optimization for performance.
        # -DNDEBUG: Define NDEBUG to disable assertions and debugging checks.
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O3;-DNDEBUG>

        # Options for MSVC compiler:
        # /O2: Optimize for speed, enabling most optimizations.
        # /DNDEBUG: Define NDEBUG to disable assertions and debugging checks.
        $<$<CXX_COMPILER_ID:MSVC>:/O2;/DNDEBUG>
    )
endif()

# Link platform-specific libraries if necessary.
# - On Windows, console I/O functions like `GetStdHandle` and `GetConsoleScreenBufferInfo`
#   (used in `repl.cpp` for terminal width) are typically provided by `kernel32.lib`.
# - On Unix-like systems, functions like `isatty` and `ioctl` are part of the standard C library
#   (libc), which is linked by default, so no explicit linking is usually required.
if (WIN32)
    target_link_libraries(logicpp PRIVATE kernel32)
endif()

# Optional: Add installation rules.
# This specifies that the 'logicpp' executable should be installed into the 'bin' directory
# within the installation prefix (e.g., `/usr/local/bin` on Linux, `C:\Program Files\LogicPP\bin` on Windows).
install(TARGETS logicpp DESTINATION bin)
