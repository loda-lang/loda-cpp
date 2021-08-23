# LODA Interpreter and Miner (C++)

LODA is an assembly language, a computational model and a tool for mining integer sequences. You can use it to search programs that calculate integer sequences from the [On-Line Encyclopedia of Integer Sequences®](http://oeis.org/) (OEIS®).

This repository ([loda-cpp](https://github.com/loda-lang/loda-cpp)) contains an implementation of the [LODA language](https://github.com/loda-lang/loda-lang) in C++ including an interpreter, an optimizer and a miner. The miner is used to generate LODA programs for integer sequences from the OEIS which are stored in [loda-programs](https://github.com/loda-lang/loda-programs).

## Installation

There are currently no binaries available. You need to build it using the `make` tool and a standard C++ compiler. It has been tested on Linux and MacOS and does not require any external libraries, but only the `wget`, `gzip` command-line tools. To build it, you need to run the following commands:

```bash
git clone git@github.com:loda-lang/loda-cpp.git
cd loda-cpp/src && make && cd ..
```

To do a simple test, you can run `./loda eval A000045` to calculate the first terms of the Fibonacci sequence. You can also add the binary to your system path to be able to call it from any directory.

To mine programs for OEIS sequences, you should create a fork of the [loda-programs](https://github.com/loda-lang/loda-programs) repository, clone it and let the `loda` tool know where you installed it:

1. Create a fork of [loda-programs](https://github.com/loda-lang/loda-programs) repository
2. Clone your fork: `git clone git@github.com:<YOUR-FORK>/loda-programs.git`
3. Add this environment variable to your shell configuration: `export LODA_PROGRAMS_HOME=<loda-programs-path>`

## Usage

The `loda` command-line tool provides the following commands and options:

```
Usage:                   loda <command> <options>

=== Core commands ===
  evaluate <file/seq-id> Evaluate a program to a sequence (cf. -t,-b,-s)
  optimize <file>        Optimize a program and print it
  minimize <file>        Minimize a program and print it (cf. -t)
  generate               Generate a random program and print it

=== OEIS commands ===
  mine                   Mine programs for OEIS sequences (cf. -i)
  match <file>           Match a program to OEIS sequences (cf. -i)
  check <seq-id>         Check a program for an OEIS sequence (cf. -b)
  maintain               Maintain all programs for OEIS sequences

=== Command-line options ===
  -t <number>            Number of sequence terms (default: 10)
  -b <number>            Print result in b-file format from a given offset
  -s                     Evaluate program to number of execution steps
  -c <number>            Maximum number of interpreter cycles (no limit: -1)
  -m <number>            Maximum number of used memory cells (no limit: -1)
  -p <number>            Maximum physical memory in MB (default: 1024)
  -l <string>            Log level (values: debug,info,warn,error,alert)
  -k <string>            Configuration file (default: loda.json)
  -i <string>            Name of miner configuration from loda.json

=== Environment variables ===
LODA_PROGRAMS_HOME       Directory for mined programs (default: programs)
LODA_UPDATE_INTERVAL     Update interval for OEIS metadata in days (default: 1)
LODA_MAX_CYCLES          Maximum number of interpreter cycles (same as -c)
LODA_MAX_MEMORY          Maximum number of used memory cells (same as -m)
LODA_MAX_PHYSICAL_MEMORY Maximum physical memory in MB (same as -p)
```

### Core Commands

#### evaluate (eval)

Evaluate a LODA program to an integer sequence. Takes a path to a program (`.asm` file) or the ID an OEIS sequence as argument. For example, run `./loda eval A000045` to generate the first terms of the Fibonacci sequence. You can use the option `-t` to set the number of terms, the option `-b <offset>` to generate it row-by-row in the OEIS b-file format, and `-c -1` to use an unbounded number of execution cycles (steps).

#### optimize (opt)

Optimize a LODA program and print the optimized version. The optimization is based on a static code analysis and does not involve any program evaluation. It is guaranteed to be semantics preserving for the entire integer sequence.

#### minimize (min)

Minimize a LODA program and print the minimized version. The minimization includes an optimization and additionally a brute-force removal of operations based on trial and error. It guarantees that the generated integer sequence is preserved, but only up to the number of terms specified using `-t`. In contrast to optimization, minimization is not guaranteed to be semantics preserving for the entire sequences. In practice, it yields much shorter programs than optimization and we usually apply it with a larger number of terms to increase the probability of correctness.

#### generate (gen)

Generate a random LODA program and print it. Multiple generators are supported and configured in [loda.json](loda.json). The generators use statistics from the existing programs stored. This operation is mainly used for testing the generators and should not be used to generate large amounts of programs.

### OEIS Commands

#### mine

Mine programs for OEIS integer sequences. It generates programs in a loop and tries to match them to sequences. If a match was found, an alert is printed and the program is automatically saved to the `loda-programs` folder. The miner configurations are defined in [loda.json](loda.json). Depending on the configuration, programs overwritten if they are faster. This refers to the number of execution steps needed to calculate the sequence. 

The `loda` tool is single-threaded and therefore uses one CPU during mining. It supports multiple process instances for parallel mining. You can try the [mine_parallel.sh](mine_parallel.sh) script for this.

You need an Internet connection to access the OEIS database to run this command. Downloaded files are cached in the `$HOME/.loda` folder. You can also configure a Twitter client to get notified when a match was found.

If you found new or better programs for OEIS sequences, please open a pull request in [loda-programs](https://github.com/loda-lang/loda-programs) to include it.

#### match

Match a program against the OEIS database and add it to `loda-programs`. If you manually wrote a program, you can use `loda match -i update <your-program.asm>`
to find matches in tge OEIS and to add it to the programs folders. If you wrote new programs for OEIS sequences, please open a pull request in [loda-programs](https://github.com/loda-lang/loda-programs) to include it.

#### check

Check if a program for an OEIS sequence is correct. For interactive output, use `-b 1` to enable b-file printing. Use `-c -1` to allow an unlimited number of execution cycles.

#### maintain

Run a maintenance for all programs. This checks the correctness of all programs in a random order. The programs must generate the first 100 terms of the sequence. In addition, up to the first 2000 terms are taken into account if the program is correct. Incorrect programs are removed and correct programs are minimized (see the `minimize` command). In addition, the description of the sequence in the comment of the program is updated to the latest version of the OEIS database. The program statistics and program lists are regenerated. 

## Related Repositories

* [loda-programs](https://github.com/loda-lang/loda-programs): Mined LODA programs for OEIS sequences
* [loda-rust](https://github.com/loda-lang/loda-rust): Interpreter and web interface written in Rust
* [loda-lang](https://github.com/loda-lang/loda-lang): LODA language specification
