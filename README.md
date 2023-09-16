# LODA Interpreter and Miner (C++)

[LODA](https://loda-lang.org) is an assembly language, a computational model and a tool for mining integer sequences.
You can use it to mine programs that compute sequences from the [On-Line Encyclopedia of Integer Sequences®](http://oeis.org/) (OEIS®).

This repository ([loda-cpp](https://github.com/loda-lang/loda-cpp)) contains an implementation of the [LODA language](https://loda-lang.org/spec) in C++ including an interpreter, an optimizer and a miner. The miner is used to generate LODA programs for integer sequences from the OEIS which are stored in [loda-programs](https://github.com/loda-lang/loda-programs).

## Getting Started

To install LODA using binaries, please follow the [official installation instructions](http://loda-lang.org/install/). To do a simple test, you can run `loda eval A000045` to calculate the first terms of the Fibonacci sequence. Run `loda setup` to intialize the LODA home directory. To mine programs for OEIS sequences, you can run `loda mine` (single-core) or `loda mine -p` (multi-core).

## Usage

The `loda` command-line tool provides the following commands and options:

```
Welcome to LODA developer version. More information at https://loda-lang.org/

Usage: loda <command> <options>

Core Commands:
  evaluate  <program>  Evaluate a program to an integer sequence (see -t,-b,-s)
  export    <program>  Export a program print result (see -o)
  optimize  <program>  Optimize a program and print it
  minimize  <program>  Minimize a program and print it (see -t)
  profile   <program>  Measure program evaluation time (see -t)
  fold <program> <id>  Fold a subprogram given by ID into a seq operation
  unfold    <program>  Unfold the first seq operation of a program

OEIS Commands:
  mine                 Mine programs for OEIS sequences (see -i,-p,-P,-H)
  check  <program>     Check a program for an OEIS sequence (see -b)
  mutate <program>     Mutate a program and mine for OEIS sequences
  submit <file> [id]   Submit a program for an OEIS sequence

Admin Commands:
  setup                Run interactive setup to configure LODA
  update               Run non-interactive update of LODA and its data

Targets:
  <file>               Path to a LODA file (file extension: *.asm)
  <id>                 ID of an OEIS integer sequence (example: A000045)
  <program>            Either an <file> or an <id>

Options:
  -t <number>          Number of sequence terms (default: 8)
  -b                   Print result in b-file format from offset 0
  -B <number>          Print result in b-file format from a custom offset
  -o <string>          Export format (formula,loda,pari)
  -d                   Export with dependencies to other programs
  -s                   Evaluate program to number of execution steps
  -c <number>          Maximum number of interpreter cycles (no limit: -1)
  -m <number>          Maximum number of used memory cells (no limit: -1)
  -z <number>          Maximum evaluation time in seconds (no limit: -1)
  -l <string>          Log level (values: debug,info,warn,error,alert)
  -i <string>          Name of miner configuration from miners.json
  -p                   Parallel mining using default number of instances
  -P <number>          Parallel mining using custom number of instances
  -H <number>          Number of mining hours (default: unlimited)
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

To mine programs for OEIS integer sequences, use the `loda mine` command. It generates programs in a loop and tries to match them to integer sequences. If a match was found, an alert is printed and the program is automatically saved to the local programs folder. If you are running in client mode, the programs are also submitted to a central API server and integrated into the global [loda-programs](https://github.com/loda-lang/loda-programs) repository.

By default, every miner uses one CPU. If you want to use more CPUs, you can run `loda mine -p` which will spawn multiple miner instances, each running as a separate process. You can configure the number of miner instances by running `loda setup`.

There are multiple miner profiles defined in [miners.json](miners.default.json). You can choose a miner profile using the `-i` option, for example `loda mine -i mutate`.

#### check

Check if a program for an OEIS sequence is correct. For interactive output, use `-b 1` to enable b-file printing. Use `-c -1` to allow an unlimited number of execution cycles.
