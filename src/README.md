# Development

## Source Code Structure

The C++ source code contain is organized into the following modules:

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

LODA supports Linux, macOS, and Windows. You need a standard C++ compiler and `make` (or `nmake` on Windows). No external libraries are required, but the `curl` and `gzip` command-line tools must be available.

To build from source, switch to the `src/` folder and run the appropriate command for your platform:

* Linux x86\_64: `make -f Makefile.linux-x86.mk`
* Linux ARM64: `make -f Makefile.linux-arm64.mk`
* MacOS x86\_64: `make -f Makefile.macos-x86.mk`
* MacOS ARM64: `make -f Makefile.macos-arm64.mk`
* Windows: `nmake /F Makefile.windows.mk`

After building, the `loda` executable will be copied or symlinked into the main project folder.

## Testing

Run tests from the main folder using the following commands:

- Fast tests: `./loda test-fast`
- All tests: `./loda test`
