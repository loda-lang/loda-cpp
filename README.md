# LODA Interpreter and Miner (C++)

[LODA](https://loda-lang.org) is an assembly language, a computational model, and toolset for discovering and analyzing integer sequences. It is especially useful for _mining_ programs that generate sequences from the [On-Line Encyclopedia of Integer Sequences® (OEIS®)](http://oeis.org/).

This repository, [loda-cpp](https://github.com/loda-lang/loda-cpp), provides a C++ implementation of the [LODA language](https://loda-lang.org/spec), including an interpreter, optimizer, and miner. The miner can automatically generate LODA programs for OEIS sequences, which are stored in the [loda-programs](https://github.com/loda-lang/loda-programs) repository.

## Getting Started

To install LODA, please follow the [official installation instructions](http://loda-lang.org/install/).
For a quick test, run `loda eval A000045` to compute the first terms of the Fibonacci sequence.
Initialize your LODA home directory with `loda setup`.
To mine programs for OEIS sequences, use `loda mine` (single-core) or `loda mine -p` (multi-core).

## Usage

The `loda` command-line tool supports the following commands and options:

```
Welcome to LODA developer version. More information at https://loda-lang.org/

Usage: loda <command> <options>

Core Commands:
  evaluate  <program>  Evaluate a program to an integer sequence (see -t,-b,-s)
  export    <program>  Export a program print result (see -o,-t)
  optimize  <program>  Optimize a program and print it
  minimize  <program>  Minimize a program and print it (see -t)
  profile   <program>  Measure program evaluation time (see -t)
  fold <program> <id>  Fold a subprogram given by ID into a seq operation
  unfold    <program>  Unfold the first seq operation of a program

OEIS Commands:
  mine                 Mine programs for OEIS sequences (see -i,-p,-P,-H)
  check     <program>  Check a program for an OEIS sequence (see -b)
  mutate    <program>  Mutate a program and mine for OEIS sequences
  submit  <file> [id]  Submit a program for an OEIS sequence

Admin Commands:
  setup                Run interactive setup to configure LODA
  update               Update OEIS and program data (no version upgrade)
  upgrade              Check for and install the latest LODA version

Targets:
  <file>               Path to a LODA file (file extension: *.asm)
  <id>                 ID of an OEIS integer sequence (example: A000045)
  <program>            Either an <file> or an <id>

Options:
  -t <number>          Number of sequence terms (default: 8)
  -b                   Print result in the OEIS b-file format
  -o <string>          Export format (formula,loda,pari,range)
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

Evaluate a LODA program to produce an integer sequence. You can specify either the path to a program file (`.asm`) or the ID of an integer sequence as the argument. For example, run `loda eval A000045` to generate the first terms of the Fibonacci sequence. Use the `-t` option to set the number of terms, `-b` to output the sequence row-by-row in the OEIS b-file format, and `-c -1` to allow unlimited execution cycles (steps).

#### optimize (opt)

Optimize a LODA program and print the optimized version. Optimization is performed using static code analysis and does not require program execution. This process is guaranteed to preserve the semantics of the entire integer sequence.

#### minimize (min)

Minimize a LODA program and print the minimized version. Minimization first applies optimization, then attempts to further shorten the program by removing operations through brute-force trial and error. The resulting program is guaranteed to produce the same integer sequence, but only up to the number of terms specified with `-t`. Unlike optimization, minimization does not guarantee semantic preservation for the entire sequence, but it often yields much shorter programs. For best results, use a larger number of terms to increase the likelihood of correctness.

#### profile

Measure the evaluation time of a LODA program. The `profile` command runs the specified program and reports how long it takes to compute the sequence terms. This is useful for benchmarking program performance and identifying bottlenecks. You can use the `-t` option to set the number of terms to evaluate.

#### setup

Run the interactive configuration wizard.

### OEIS Commands

#### mine

Use the `loda mine` command to automatically search for LODA programs that generate OEIS integer sequences. The miner continuously generates and tests candidate programs, saving any successful matches to your local programs folder. In client mode, discovered programs are also submitted to a central API server and integrated into the global [loda-programs](https://github.com/loda-lang/loda-programs) repository.

By default, mining uses a single CPU core. To utilize multiple cores, run `loda mine -p` to launch several miner instances in parallel. You can adjust the number of instances by running `loda setup` or using the `-P` option.

Several miner profiles are defined in [miners.json](miners.default.json). Select a profile with the `-i` option, for example: `loda mine -i mutate`.

#### check

Verify the correctness of a program for a given OEIS sequence. For interactive output, use `-b` to enable b-file printing. Use `-c -1` to allow unlimited execution cycles.

#### submit

Submit a LODA program for an integer sequence to the central repository. The `submit` command uploads your program file for review and inclusion in the global [loda-programs](https://github.com/loda-lang/loda-programs) collection. This helps share new or improved programs with the LODA community. The command automatically verifies your program for correctness before the submission.
