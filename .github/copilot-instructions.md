# GitHub Copilot Instructions for `loda-cpp`

## Project Overview

LODA is an open-source project focused on the systematic discovery and analysis of programs and formulas for integer sequences, especially those from the [On-Line Encyclopedia of Integer Sequences (OEIS)](https://oeis.org/). This repository contains the C++ implementation of the LODA language. It includes, among others, an interpreter for evaluation programs and generating integer sequences, and a miner to search for programs that generate sequences from the OEIS.

## Source Code Structure

The C++ source code organized under the `src/` directory into the following modules:

- [`base`](../src/base): Basic data structures (e.g., UID)
- [`cmd`](../src/cmd): Command-line interface, test suite, and main entry point
- [`eval`](../src/eval): Interpreter and evaluation engine for LODA programs
- [`form`](../src/form): Formula generation (including PARI/GP integration)
- [`lang`](../src/lang): LODA language core (parser, analyzer, program representation)
- [`math`](../src/math): Internal math library (big numbers, integer sequences)
- [`mine`](../src/mine): Mining infrastructure (generators, matchers, miners)
- [`seq`](../src/seq): Sequence data management and OEIS integration
- [`sys`](../src/sys): System utilities (file I/O, git, logging, setup, web client)

## Building

LODA supports Linux, macOS, and Windows. You need a standard C++ compiler and `make` (or `nmake` on Windows). No external libraries are required, but the `curl` and `gzip` command-line tools must be available at runtime.

To build from source, switch to the `src/` folder and run the appropriate command for your platform:

* Linux x86\_64: `make -f Makefile.linux-x86.mk`
* Linux ARM64: `make -f Makefile.linux-arm64.mk`
* MacOS x86\_64: `make -f Makefile.macos-x86.mk`
* MacOS ARM64: `make -f Makefile.macos-arm64.mk`
* Windows: `nmake /F Makefile.windows.mk`

After building, the `loda` executable will be copied or symlinked into the main project folder.

## Testing

Run tests from the main project folder using the following commands:
- Fast tests: `./loda test-fast`
- All tests: `./loda test`

## Code Style Guidelines

- C++ Standard: C++17
- Compiler flags: strict compilation with `-Wall -Werror` (all warnings are treated as errors).
- Generally follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for formatting.
- Naming conventions:
  - Classes: PascalCase (e.g., `MineManager`, `Evaluator`)
  - Functions/methods: camelCase (e.g., `evaluate()`, `checkProgram()`)
  - Member variables: snake_case with no prefix (e.g., `max_memory`, `num_terms`)
  - Constants: UPPER_SNAKE_CASE (e.g., `PREFIX_SUBMITTED_BY`)
- File organization:  
  - Each class should have a dedicated `.hpp` header and `.cpp` implementation file.
  - Group related classes in module folders (e.g., `eval/`, `lang/`, `mine/`).

## Key Concepts

### Programs

LODA programs are represented by the [`Program`](../src/lang/program.hpp) class. A program consists of a list of operations, each described by the `Operation` class. Each operation has an `opcode` (specifying the operation type), a target operand and a source operand, both represented by the `Operand` class.

Most operations are arithmetic and involve two operands. The source operand is always read-only, while the target operand can be modified. New operation types can be added by extending the relevant classes and updating the semantics engine.

The full language specification is available at [loda-lang.org/spec](https://loda-lang.org/spec/), detailing syntax, semantics, and supported operations.

LODA program files use the `.asm` extension.

### Sequences

Integer sequences are represented by the [`Sequence`](../src/math/sequence.hpp) class. The project integrates with OEIS to fetch sequence definitions and find LODA programs that compute those sequences.
All programs for OEIS integer sequences are stored in a separate repository: [loda-programs](https://github.com/loda-lang/loda-programs).

### Semantics and Evaluation

Arithmetic operations are implemented in the [`Semantics`](../src/eval/semantics.hpp) class. The core execution of LODA programs is handled by the [`Interpreter`](../src/eval/interpreter.hpp) class, which takes a [`Program`](../src/lang/program.hpp) and operates on a runtime state represented by the [`Memory`](../src/eval/memory.hpp) class. Program evaluation and integer sequence generation are performed by the [`Evaluator`](../src/eval/evaluator.hpp) class. There are a few specialized evaluator implementations such as [`IncrementalEvaluator`](../src/eval/evaluator_inc.hpp) for performance optimization and advanced analysis.

### Mining

The mining process generates random programs and matches them against OEIS sequences. Key classes:
- [`Miner`](../src/mine/miner.hpp): Coordinates the mining process
- [`Generator`](../src/mine/generator.hpp): Creates random programs (multiple versions: V1-V8)
- [`Matcher`](../src/mine/matcher.hpp): Matches program output against sequences
- [`MineManager`](../src/mine/mine_manager.hpp): Manages program and sequence data

### Formula Generation

Automated formula generation from LODA programs is used to find closed-form or recurrence relations for OEIS sequences. It is implemented using the following key classes in the `form` module:
- [`Expression`](../src/form/expression.hpp): Represents mathematical expressions and sub-expressions.
- [`Formula`](../src/form/formula.hpp): Encapsulates a complete formula, including recurrence relations and initial terms.
- [`FormulaGenerator`](../src/form/formula_generator.hpp): Converts a LODA program into a formula.

## Common Tasks

### Adding a New Command

1. Declare the command in [`cmd/commands.hpp`](../src/cmd/commands.hpp).
2. Implement it in [`cmd/commands.cpp`](../src/cmd/commands.cpp).
3. Add command-line parsing in [`cmd/main.cpp`](../src/cmd/main.cpp).
4. For official commands only: update help text in `Commands::help()` and [README.md](../README.md).

### Adding a New Operation Type

1. Add the new operation type to the relevant enums, declarations and metadata in [`lang/program.hpp`](../src/lang/program.hpp) and [`lang/program.cpp`](../src/lang/program.cpp).
2. For arithmetic operations, implement the behavior in the [`Semantics`](../src/eval/semantics.hpp) class and [`eval/semantics.cpp`](../src/eval/semantics.cpp).
3. For arithmetic operation, extend [`eval/range_generator.cpp`](../src/eval/range_generator.cpp) to support range computation for the new operation, if possible.
4. Check if the new operation needs to be handled in utility or mining code:
    - [`lang/program_util.cpp`](../src/lang/program_util.cpp)
    - [`mine/iterator.cpp`](../src/mine/iterator.cpp)
    - [`mine/generator.cpp`](../src/mine/generator.cpp)
5. Create a CSV file in `tests/semantics/<opcode>.csv` containing test cases (three columns: operand1, operand2, expected result).
6. Run the fast test suite: `./loda test-fast` to verify correctness.
7. If static code optimizations are possible for the new operation, extend [`eval/optimizer.cpp`](../src/eval/optimizer.cpp). Add relevant `.asm` test files to `tests/optimizer`.

### Adding Tests

Add test methods to the [`Test`](../src/cmd/test.hpp) class and implement in [`cmd/test.cpp`](../src/cmd/test.cpp). For most common tasks, it suffices to add corresponding test data in the `tests` folder and execute the existing tests.

## Dependencies

The project has minimal external dependencies:
- Standard C++ library
- `curl` (command-line tool for HTTP requests)
- `gzip` (for compression)
- Optional: `pari-gp` (for PARI formula testing)

## General Guidelines for Changes

- Make minimal, focused changes and match the style of existing code.
- Use RAII for resource management; avoid manual memory allocation.
- Use either logging and its error handling or throw runtime exceptions consistently for each class.
- Optimize for performance and memory safety, especially in mining and evaluation code. Prefer algorithmic optimizations over low-level tuning.
- Run the full test suite (`./loda test`) after any modification to ensure correctness.
- Document enhancements and bugfixes in `CHANGELOG.md`.
- Aim to maintain backward compatibility, but it is not strictly required.
- Update the [LODA language specification](https://loda-lang.org/spec/) for any language changes.
