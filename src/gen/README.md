# Program Generation

The source code in this folder contains generators for LODA programs and related utilities. Generators can produce programs either randomly or deterministically, which are then used in the mining process to find programs that match integer sequences from the OEIS database.

## Overview

There are multiple generators available with differing strategies. They can generate programs either randomly or deterministically. Random program generators can use statistics on existing LODA programs to define probability distributions. We also distinguish between generators that create programs from scratch, those that mutate randomly chosen existing programs, and those that use patterns or templates to create programs.

All generators implement the [Generator](generator.hpp) interface. They are identified using a plain version number and instantiated using a factory class.

## Generator Interface

The base `Generator` class provides:
- Abstract methods for program generation: `generateProgram()` and `generateOperation()`
- Configuration options via the `Generator::Config` class
- Factory pattern for instantiating specific generator versions
- Post-processing utilities for fixing program issues (causality, singularities, loops, etc.)

## Program Iterator

The [Iterator](iterator.hpp) class provides deterministic program enumeration. It can iterate through programs in a systematic way, similar to incrementing a number. This is useful for exhaustive, brute-force searches.

## Blocks

The [Blocks](blocks.hpp) class manages collections of program fragments (blocks) extracted from existing programs. These blocks can be used to construct new programs by combining them probabilistically.

## Available Generators

### Generator Version 1

[GeneratorV1](generator_v1.cpp) produces random programs from scratch or from existing program templates. It includes several configuration options, for example, to control the approximate length of the programs and the used operation and operand types. For constants, it uses a distribution derived from the program stats. Historically, this generator found many simple programs in the early days of the project. However, nowadays it hardly finds new programs. This is mainly because it is stateless: it does not use any probabilistic model for dependencies between multiple operations in a generated program.

```json
{
    "name": "my-generator",
    "version": 1,
    "length": 40,
    "maxIndex": 8,
    "loops": false,
    "calls": false,
    "indirectAccess": false,
    "template": [
        "$LODA_HOME/programs/templates/call2.asm",
        "$LODA_HOME/programs/templates/loop.asm",
      ]
}
```

### Generator Version 2

[GeneratorV2](generator_v2.cpp) generates random programs from scratch. It uses probability distributions generated from the program stats to randomly select the program length and operation (including operand). It does not include any configuration options and solely depends on the program stats. It is also stateless: it does not use any probability distributions for sequential dependencies of operations.

### Generator Version 3

[GeneratorV3](generator_v3.cpp) also uses program length and operation distributions. However, it uses separate distributions per position in the program. For example, it uses different distributions for the first and the second operation in the generated programs.

### Generator Version 4

[GeneratorV4](generator_v4.cpp) is partially deterministic and uses persistence to save its state to disk. It first initializes around 200 random programs of varying length and complexity and stores them on disk. It uses these random programs as *seeds* for deterministic generation using a program iterator. In a nutshell, the iterator increases a program like a number. Hence this is an exhaustive, brute-force search.

### Generator Version 5

[GeneratorV5](generator_v5.cpp) splits up the existing programs into blocks of a few operations and uses probability distributions to randomly select and combine such program blocks.

### Generator Version 6

[GeneratorV6](generator_v6.cpp) is the most successful generator at the time of writing. It works quite simply: it randomly selects one of the already existing programs and mutates them, i.e., modifies, adds or deletes operations. It includes a parameter for controlling the mutation rate. In addition, it does not use a uniform distribution for selecting programs to mutate, but it takes into account the git history of the programs repository. In a nutshell, it prefers to mutate recently added or modified programs.

```json
{
    "name": "my-generator",
    "version": 6,
    "mutationRate": 0.2
}
```

### Generator Version 7

[GeneratorV7](generator_v7.cpp) mutates [program patterns](https://github.com/loda-lang/loda-programs/tree/main/patterns) that have been generated using [loda-rust](https://github.com/loda-lang/loda-rust).

### Generator Version 8

[GeneratorV8](generator_v8.cpp) is not a true generator, but rather an input plugin that reads programs from a batch file. The batch file must have the following format: every line corresponds to one program, where operations are separated using a semi-colon. This can be used for [NMT Mining](https://github.com/loda-lang/nmt-mining).

```json
{
    "name": "my-generator",
    "version": 8,
    "batchFile": "programs.txt"
}
```
