# GitHub Copilot Instructions for `loda-cpp`

## Project Overview

LODA is an open-source project focused on the systematic discovery and analysis of programs and formulas for integer sequences, especially those from the [On-Line Encyclopedia of Integer Sequences (OEIS)](https://oeis.org/). This repository contains the C++ implementation of the LODA language. It includes, among others, an interpreter for evaluation programs and generating integer sequences, and a miner to search for programs that generate sequences from the OEIS.

## Source Code Structure

The C++ source code organized under the `src/` directory into the following modules:

- [`base`](base): Basic data structures (e.g., UID)
- [`cmd`](cmd): Command-line interface, test suite, and main entry point
- [`eval`](eval): Interpreter and evaluation engine for LODA programs
- [`form`](form): Formula generation (including PARI/GP integration)
- [`lang`](lang): LODA language core (parser, analyzer, program representation)
- [`math`](math): Internal math library (big numbers, integer sequences)
- [`mine`](mine): Mining infrastructure (generators, matchers, miners)
- [`seq`](seq): Sequence data management and OEIS integration
- [`sys`](sys): System utilities (file I/O, git, logging, setup, web client)

## Building

LODA supports Linux, macOS, and Windows. You need a standard C++ compiler and `make` (or `nmake` on Windows). No external libraries are required, but the `curl` and `gzip` command-line tools must be available at runtime.

To build from source, switch to the `src/` folder and run the appropriate command for your platform:

* Linux x86\_64: `cd src && make -f Makefile.linux-x86.mk`
* Linux ARM64: `cd src && make -f Makefile.linux-arm64.mk`
* MacOS x86\_64: `cd src && make -f Makefile.macos-x86.mk`
* MacOS ARM64: `cd src && make -f Makefile.macos-arm64.mk`
* Windows: `cd src && nmake /F Makefile.windows.mk`

After building, the `loda` executable will be copied or symlinked into the main project folder.

## Testing

Run tests from the main folder using the following commands:
- Fast tests: `./loda test-fast`
- All tests: `./loda test`

## Code Style Guidelines

- C++ Standard: C++17
- Compiler Flags: strict compilation with `-Wall -Werror` (all warnings are treated as errors).
- Naming Conventions:
  - Classes: PascalCase (e.g., `MineManager`, `Evaluator`)
  - Functions/methods: camelCase (e.g., `evaluate()`, `checkProgram()`)
  - Member variables: snake_case with no prefix (e.g., `max_memory`, `num_terms`)
  - Constants: UPPER_SNAKE_CASE (e.g., `PREFIX_SUBMITTED_BY`)
- File Organization:  
  - Each class should have a dedicated `.hpp` header and `.cpp` implementation file.
  - Group related classes in module folders (e.g., `eval/`, `lang/`, `mine/`).

## Key Concepts

### Programs

LODA programs are represented by the `Program` class in `lang/program.hpp`.  
A program consists of a sequence of operations, each described by the `Operation` class.  
Each operation has an `opcode` (specifying the operation type), a target operand and a source operand, both represented by the `Operand` class.

Most operations are arithmetic and involve two operands. The source operand is always read-only, while the target operand can be modified. New operation types can be added by extending the relevant classes and updating the semantics engine.

The full language specification is available at [loda-lang.org/spec](https://loda-lang.org/spec/), detailing syntax, semantics, and supported operations.

LODA program files use the `.asm` extension.

### Sequences

Integer sequences are represented by the `Sequence` class (in `math/sequence.hpp`). The project integrates with OEIS to fetch sequence definitions and find LODA programs that compute those sequences.
All programs for OEIS integer sequences are stored in a separate repository: [loda-programs](https://github.com/loda-lang/loda-programs).

### Semantics and Evaluation

Arithmetic operations are implemented in the `Semantics` class (`eval/semantics.hpp`).  
The core execution of LODA programs is handled by the `Interpreter` class (`eval/interpreter.hpp`), which takes a `Program` and operates on a runtime state represented by the `Memory` class (`eval/memory.hpp`).
Program evaluation and integer sequence generation are performed by the `Evaluator` class (`eval/evaluator.hpp`).  
There are a few specialized evaluator implementations such as `IncrementalEvaluator` (`eval/evaluator_inc.hpp`) for performance optimization and advanced analysis.

### Mining

The mining process generates random programs and matches them against OEIS sequences. Key classes:
- `Miner`: Coordinates the mining process
- `Generator`: Creates random programs (multiple versions: V1-V8)
- `Matcher`: Matches program output against sequences
- `MineManager`: Manages program and sequence data

### Formula Generation

Automated formula generation from LODA programs is used to find closed-form or recurrence relations for OEIS sequences. It is implemented using the following key classes in the `form` module:
- `Expression` (`form/expression.hpp`): Represents mathematical expressions and sub-expressions.
- `Formula` (`form/formula.hpp`): Encapsulates a complete formula, including recurrence relations and initial terms.
- `FormulaGenerator` (`form/formula_generator.hpp`): Converts a LODA program into a formula.

## Common Tasks

### Adding a New Command

1. Declare the command in `cmd/commands.hpp`.
2. Implement it in `cmd/commands.cpp`.
3. Add command-line parsing in `cmd/main.cpp`.
4. For official commands only: update help text in `Commands::help()` and [README.md](../README.md).

### Adding a New Operation Type

1. Add the new operation type to the relevant enums, declarations and metadata in `lang/program.hpp` and `lang/program.cpp`.
2. For arithmetic operations, implement the behavior in the `Semantics` class (`eval/semantics.hpp` and `eval/semantics.cpp`).
3. For arithmetic operation, extend `eval/range_generator.cpp` to support range computation for the new operation, if possible.
4. Check if the new operation needs to be handled in utility or mining code:
  - `lang/program_util.cpp`
  - `mine/iterator.cpp`
  - `mine/generator.cpp`
5. Create a CSV file in `tests/semantics/<opcode>.csv` containing test cases (three columns: operand1, operand2, expected result).
6. Run the fast test suite: `./loda test-fast` to verify correctness.
7. If static code optimizations are possible for the new operation, extend `eval/optimizer.cpp`. Add relevant `.asm` test files to `tests/optimizer`.

### Adding Tests

Add test methods to the `Test` class in `src/cmd/test.hpp` and implement in `src/cmd/test.cpp`. For most common tasks, it suffices to add corresponding test data in the `tests` folder and execute the existing tests.

## Dependencies

The project has minimal external dependencies:
- Standard C++ library
- `curl` (command-line tool for HTTP requests)
- `gzip` (for compression)
- Optional: `pari-gp` (for PARI formula testing)

## General Guidelines for Changes

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for formatting.
- Make minimal, focused changes and match the style of existing code.
- Consistently use RAII for resource management.
- For every class, consistently use either logging and it's error handling, or throw runtime exceptions in case of errors.
- Optimize for performance, especially in mining and evaluation code.
- Algorithmic optimizations are more important than low-level performance tuning.
- Run the full test suite (`./loda test`) after any modification.
- Document enhancements and bugfixes in `CHANGELOG.md`.
- We aim at staying backwward compatible, but it is not strinctly required. Changes must be reflected in the LODA language specification: https://loda-lang.org/spec/
