# LODA Tool (C++ Implementation)

[LODA](https://loda-lang.org) is an open-source assembly language and toolset for the discovery, mining, and analysis of integer sequences — especially those from the [On-Line Encyclopedia of Integer Sequences® (OEIS®)](https://oeis.org/).

This repository, [loda-cpp](https://github.com/loda-lang/loda-cpp), provides a C++ implementation of the [LODA language](https://loda-lang.org/spec), including an interpreter, optimizer, formula generator and a miner. The miner can automatically generate LODA programs for OEIS sequences, which are stored in the [loda-programs](https://github.com/loda-lang/loda-programs) repository.

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

Commands:
  eval      <program>  Evaluate an integer sequence program (see -t,-b,-s)
  check     <program>  Verify correctness of an integer sequence program (see -b)
  mine                 Mine programs for integer sequences (see -i,-p,-P,-H)
  submit  <file> [id]  Submit an integer sequence program to the central repository
  export    <program>  Export a program and print the result (see -o,-t)
  optimize  <program>  Optimize a program and print the result
  minimize  <program>  Minimize a program and print the result (see -t)
  profile   <program>  Measure program evaluation time (see -t)
  fold <program> <id>  Fold a subprogram given by ID into a seq-operation
  unfold    <program>  Unfold the first seq-operation of a program
  mutate    <program>  Mutate a program to mine for integer sequences
  setup                Run interactive setup to configure LODA
  update               Update integer sequence and program data
  upgrade              Check for and install the latest LODA version

Targets:
  <file>               Path to a LODA file (file extension: *.asm)
  <id>                 ID of an integer sequence (example: A000045)
  <program>            Either an <file> or an <id>

Options:
  -t <number>          Number of sequence terms (default: 8)
  -b                   Print result in the OEIS b-file format
  -o <string>          Export format (formula,loda,pari,range)
  -d                   Export with dependencies to other programs
  -s                   Evaluate program and return number of execution steps
  -c <number>          Maximum number of execution steps (no limit: -1)
  -m <number>          Maximum number of used memory cells (no limit: -1)
  -z <number>          Maximum evaluation time in seconds (no limit: -1)
  -l <string>          Log level (values: debug,info,warn,error,alert)
  -i <string>          Name of miner configuration from miners.json
  -p                   Parallel mining using default number of instances
  -P <number>          Parallel mining using custom number of instances
  -H <number>          Number of mining hours (default: unlimited)
```

## Development

For developer guidance, see the detailed documentation in `src/README.md` and the Copilot instructions file (`.github/copilot-instructions.md`). These cover code structure, style, and common tasks for contributing to LODA.

## License

This project is licensed under the Apache License, Version 2.0. See the [LICENSE](LICENSE) file for details.
