# Development

## Building

You need the `make` (`nmake` on Windows) tool and a standard C++ compiler. It is supported on Linux, macOS and Windows. It does not require any external libraries, but only the `curl`, `gzip` command-line tools. To build it, switch to the `src` folder and run the build command for your platform:

* Linux x86\_64: `make -f Makefile.linux-x86.mk`
* Linux ARM64: `make -f Makefile.linux-arm64.mk`
* macOS Intel: `make -f Makefile.macos-x86.mk`
* macOS M1: `make -f Makefile.macos-arm64.mk`
* Windows: `nmake /F Makefile.windows.mk`

## Modules

The source code is organized into these modules:

* [cmd](cmd): Commands, tests and main.
* [eval](eval): Interpreter and evaluation of programs.
* [form](form): Formula generation from programs.
* [lang](lang): LODA language core including interpreter.
* [math](math): Internal maths library.
* [mine](mine): Mining programs for OEIS sequences.
* [oeis](oeis): Utils for managing OEIS data.
* [sys](sys): System utils.
