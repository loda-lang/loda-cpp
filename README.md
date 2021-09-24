# LODA Interpreter and Miner (C++)

[LODA](https://loda-lang.org) is an assembly language, a computational model and a tool for mining integer sequences.
You can use it to mine programs that compute sequences from the [On-Line Encyclopedia of Integer Sequences®](http://oeis.org/) (OEIS®).

This repository ([loda-cpp](https://github.com/loda-lang/loda-cpp)) contains an implementation of the [LODA language](https://loda-lang.org/spec) in C++ including an interpreter, an optimizer and a miner. The miner is used to generate LODA programs for integer sequences from the OEIS which are stored in [loda-programs](https://github.com/loda-lang/loda-programs).

## Installation

To install LODA using binaries, please follow the [official installation instructions](http://loda-lang.org/install/).

Alternatively, you can also build it from sources. You need the `make` tool and a standard C++ compiler. It has been tested on Linux and macOS and does not require any external libraries, but only the `curl`, `gzip` command-line tools. To build it, you need to run the following commands:

```bash
git clone git@github.com:loda-lang/loda-cpp.git
cd loda-cpp/src && make && cd ..
./loda setup
```

To do a simple test, you can run `loda eval A000045` to calculate the first terms of the Fibonacci sequence.

To mine programs for OEIS sequences, you can run `loda mine` (single-core) or use the [mine_parallel.sh](mine_parallel.sh) script (multi-core).

## Usage

The `loda` command-line tool provides the following commands and options:

```
Welcome to LODA developer version. More information at https://loda-lang.org

Usage: loda <command> <options>

Core Commands:
  evaluate <program>  Evaluate a program to an integer sequence (see -t,-b,-s)
  optimize <program>  Optimize a program and print it
  minimize <program>  Minimize a program and print it (see -t)
  setup               Run interactive setup to configure LODA

OEIS Commands:
  mine [asm-file]     Mine programs for OEIS sequences (see -i)
                      (optional argument is an initial program)
  match <asm-file>    Match a program to OEIS sequences (see -i)
  check <seq-id>      Check a program for an OEIS sequence (see -b)
  maintain            Maintain all programs for OEIS sequences

Targets:
  <asm-file>          Path to a LODA file (file extension: *.asm)
  <seq-id>            ID of an OEIS integer sequence (example: A000045)
  <program>           Either an <asm-file> or a <seq-id>

Options:
  -t <number>         Number of sequence terms (default: 10)
  -b <number>         Print result in b-file format from a given offset
  -s                  Evaluate program to number of execution steps
  -c <number>         Maximum number of interpreter cycles (no limit: -1)
  -m <number>         Maximum number of used memory cells (no limit: -1)
  -l <string>         Log level (values: debug,info,warn,error,alert)
  -i <string>         Name of miner configuration from loda.json
```

### Core Commands

#### evaluate (eval)

Evaluate a LODA program to an integer sequence. Takes a path to a program (`.asm` file) or the ID an OEIS sequence as argument. For example, run `loda eval A000045` to generate the first terms of the Fibonacci sequence. You can use the option `-t` to set the number of terms, the option `-b <offset>` to generate it row-by-row in the OEIS b-file format, and `-c -1` to use an unbounded number of execution cycles (steps).

#### optimize (opt)

Optimize a LODA program and print the optimized version. The optimization is based on a static code analysis and does not involve any program evaluation. It is guaranteed to be semantics preserving for the entire integer sequence.

#### minimize (min)

Minimize a LODA program and print the minimized version. The minimization includes an optimization and additionally a brute-force removal of operations based on trial and error. It guarantees that the generated integer sequence is preserved, but only up to the number of terms specified using `-t`. In contrast to optimization, minimization is not guaranteed to be semantics preserving for the entire sequences. In practice, it yields much shorter programs than optimization and we usually apply it with a larger number of terms to increase the probability of correctness.

#### setup

Run the interactive configuration wizard.

### OEIS Commands

#### mine

To mine programs for OEIS integer sequences, use `loda mine`. It generates programs in a loop and tries to match them to sequences. If a match was found, an alert is printed and the program is automatically saved to the local programs folder. If you are running in client mode, the programs are also submitted to acentral server and incorporated into the global [loda-programs](https://github.com/loda-lang/loda-programs) repository. The miner configurations are defined in [miners.json](miners.default.json). Depending on the configuration, programs overwritten if they are faster. This refers to the number of execution steps needed to calculate the sequence. 

The `loda` tool is single-threaded and therefore uses one CPU during mining. It supports multiple process instances for parallel mining. You can use the [mine_parallel.sh](mine_parallel.sh) script for this. It automatically determines the number of processes to be used for parallel mining.

#### check

Check if a program for an OEIS sequence is correct. For interactive output, use `-b 1` to enable b-file printing. Use `-c -1` to allow an unlimited number of execution cycles.

#### maintain

Run a maintenance for all programs. This checks the correctness of all programs in a random order. The programs must generate the first 100 terms of the sequence. In addition, up to the first 2000 terms are taken into account if the program is correct. Incorrect programs are removed and correct programs are minimized (see the `minimize` command). In addition, the description of the sequence in the comment of the program is updated to the latest version of the OEIS database. The program statistics and program lists are regenerated. 

## Additional Resources

* [loda-lang.org](https://loda-lang.org): Main home page of LODA.
* [loda-programs](https://github.com/loda-lang/loda-programs): Repository of mined LODA programs for OEIS sequences.
* [loda-rust](https://github.com/loda-lang/loda-rust): Interpreter and web interface written in Rust.

## Development

If you want to contribute to `loda-cpp`, the best starting point for reading the code is probably [program.hpp](/src/include/program.hpp). It contains the model classes for programs including operands, operations and programs. You can find the implementation of all arithmetics operations in [semantics.cpp](/src/semantics.cpp). Apart from container classes for sequences and memory, the main part of the operational semantics of programs is implemented in [interpreter.cpp](/src/interpreter.cpp). The evaluation of programs to sequences is coded in [evaluator.cpp](/src/evaluator.cpp).

For mining, there are multiple generator implementations, which are used to create random programs. They are configured via [miners.json](/miners.default.json) and use statistics from the existing programs stored in the `stats` folder. To reduce and index the target sequences, we use [Matcher](/src/include/matcher.hpp) classes. They allow matching of sequences modulo additional operations such as linear transformations.

To reduce and normalize the programs we use the [Optimizer](/src/include/optimizer.hpp) and the [Minimizer](/src/include/minimizer.hpp) class.

There is a test suite in [test.cpp](/src/test.cpp) which can be executed using `loda test`. This is also automatically executed as an action in the GitHub workflow.
