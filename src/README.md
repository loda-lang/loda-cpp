# Development

## Building

To build LODA, all you need is the `make` (`nmake` on Windows) tool and a standard C++ compiler. All major platforms are supported: Linux, MacOS, and Windows. It does not require any external libraries, but only the `curl`, `gzip` command-line tools. To build from sources, switch to the `src` folder and run the build command for your platform:

* Linux x86\_64: `make -f Makefile.linux-x86.mk`
* Linux ARM64: `make -f Makefile.linux-arm64.mk`
* MacOS x86\_64: `make -f Makefile.macos-x86.mk`
* MacOS ARM64: `make -f Makefile.macos-arm64.mk`
* Windows: `nmake /F Makefile.windows.mk`

## Modules

The source code is organized into these modules:

* [base](base): Basic common data structures.
* [cmd](cmd): Commands, tests and main.
* [eval](eval): Interpreter and evaluation of programs.
* [form](form): Formula generation from programs.
* [lang](lang): LODA language core.
* [math](math): Internal math library.
* [mine](mine): Program mining.
* [seq](seq): Sequence data management.
* [oeis](oeis): Utils for managing OEIS data.
* [sys](sys): System utils.
