# GitHub Copilot Instructions for loda-cpp

## Project Overview

LODA (Language of Ordered Data Arrays) is an assembly language and computational model for discovering and analyzing integer sequences. This C++ implementation provides:
- An interpreter for the LODA language
- An optimizer and minimizer for LODA programs
- A miner that automatically generates programs for OEIS (On-Line Encyclopedia of Integer Sequences) sequences

## Architecture

The codebase is organized into the following modules under `src/`:

- **base**: Basic common data structures (UID, etc.)
- **cmd**: Command-line interface, tests, and main entry point
- **eval**: Program interpreter and evaluation engine
- **form**: Formula generation from programs (LODA, PARI, formulas)
- **lang**: LODA language core (parser, analyzer, program representation)
- **math**: Internal math library (big numbers, sequences)
- **mine**: Program mining infrastructure (generators, matchers, miners)
- **seq**: Sequence data management (OEIS integration)
- **oeis**: Utilities for managing OEIS data
- **sys**: System utilities (file I/O, logging, setup, web client)

## Building and Testing

### Build Commands

The project uses Makefiles for different platforms:
- Linux x86_64: `cd src && make -f Makefile.linux-x86.mk`
- Linux ARM64: `cd src && make -f Makefile.linux-arm64.mk`
- MacOS x86_64: `cd src && make -f Makefile.macos-x86.mk`
- MacOS ARM64: `cd src && make -f Makefile.macos-arm64.mk`
- Windows: `cd src && nmake /F Makefile.windows.mk`

### Test Commands

- Fast tests: `./loda test` or `./loda test-fast`
- All tests: `./loda test-all`
- Slow tests: `./loda test-slow`

Always run tests after making changes to ensure correctness.

## Code Style Guidelines

- **C++ Standard**: C++17
- **Compiler Flags**: `-Wall -Werror` (warnings are errors)
- **Naming Conventions**:
  - Classes: PascalCase (e.g., `MineManager`, `Evaluator`)
  - Functions/methods: camelCase (e.g., `evaluate()`, `checkProgram()`)
  - Member variables: snake_case with no prefix (e.g., `max_memory`, `num_terms`)
  - Constants: UPPER_SNAKE_CASE (e.g., `PREFIX_SUBMITTED_BY`)
- **File Organization**: Each class typically has a `.hpp` header and `.cpp` implementation file
- **Comments**: Use minimal comments; prefer self-documenting code. Use comments for complex algorithms or non-obvious behavior.

## Key Concepts

### Programs
LODA programs are represented by the `Program` class (in `lang/program.hpp`) and consist of operations that manipulate memory cells. Programs can be:
- Evaluated to generate sequence terms
- Optimized to reduce execution steps
- Minimized to reduce program size
- Folded/unfolded to use sub-programs

### Sequences
Integer sequences are represented by the `Sequence` class (in `math/sequence.hpp`). The project integrates with OEIS to fetch and validate sequences.

### Mining
The mining process generates random programs and matches them against OEIS sequences. Key classes:
- `Miner`: Coordinates the mining process
- `Generator`: Creates random programs (multiple versions: V1-V8)
- `Matcher`: Matches program output against sequences
- `MineManager`: Manages program and sequence data

### Evaluation
Programs are evaluated using the `Evaluator` class with different modes:
- Standard evaluation
- Incremental evaluation (for optimization)
- Virtual evaluation (for pattern matching)

## Common Tasks

### Adding a New Command
1. Declare the command in `src/cmd/commands.hpp`
2. Implement it in `src/cmd/commands.cpp`
3. Add command-line parsing in `src/cmd/main.cpp`
4. Update help text in `Commands::help()`

### Adding a New Generator
1. Create `generator_vX.cpp` and `generator_vX.hpp` in `src/mine/`
2. Implement the `Generator` interface
3. Register in `Generator::create()` factory method
4. Update `miners.default.json` configuration

### Adding Tests
Add test methods to the `Test` class in `src/cmd/test.hpp` and implement in `src/cmd/test.cpp`.

## Dependencies

The project has minimal external dependencies:
- Standard C++ library
- `curl` (command-line tool for HTTP requests)
- `gzip` (for compression)
- Optional: `pari-gp` (for PARI formula testing)

## Important Files

- `README.md`: User-facing documentation
- `src/README.md`: Developer documentation
- `src/mine/README.md`: Mining architecture details
- `miners.default.json`: Miner configuration
- `.github/workflows/main.yml`: CI pipeline

## Guidelines for Changes

1. **Make minimal, focused changes**: Modify only what's necessary
2. **Test thoroughly**: Run `./loda test` before committing
3. **Follow existing patterns**: Match the style and structure of existing code
4. **Update documentation**: If adding features, update relevant README files
5. **No breaking changes**: Maintain backward compatibility with existing LODA programs
6. **Performance matters**: The miner runs for hours/days; optimize hot paths
7. **Memory safety**: Use RAII; avoid manual memory management
8. **Error handling**: Use exceptions for errors; check return values

## Special Considerations

- **LODA Language Spec**: Changes must comply with the LODA language specification
- **OEIS Integration**: Sequence IDs follow OEIS format (e.g., A000045)
- **Program Files**: LODA programs use `.asm` extension
- **Backward Compatibility**: Existing programs in loda-programs repository must remain valid
- **Mining Configuration**: Changes to generators/matchers should be configurable in `miners.json`
