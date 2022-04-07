To install or update LODA, please follow the [installation instructions](https://loda-lang.org/install/). Here are the release notes:

## [Unreleased]

### Features

* Added `profile` command for measuring program evaluation time

### Enhancements

* Improve handling of programs with minimization errors/warnings
* Improve comparsion of programs supporting IE

## v22.4.5

### Bugfixes

* Fix reporting of CPU hours (active miners) in parallel mining mode

### Performance Enhancements

* Ignore updates of recently found new programs
* Reduce maximum number of terms in validation to 4000
* Omit expensive comparison for new program
* Prefer programs that support incremental evaluation

## v22.4.3

### Bugfixes

* Optimizer does not consider `$1` as reserved cell anymore (formerly output cell)
* Validator does not remove operations anymore that are needed for computing more terms
* Fix corner case in big integer comparison

### Features

* Track manually submitted programs in the miner profile metric
* `check` command accepts program files as argument (extracts A-number from comment in programs)

### Enhancements

* Improved operation sorting in optimizer
* Improved output during updates on Windows

## v22.3.11

### Features

* Parallel mining with custom number of instances: `-P <number>`
* Short-hand parameter for b-file output with offset 0: `-b`
* `submit` command extracts A-number from comment in programs

### Enhancements

* Incremental evaluation (IE) partially supports programs with non-commutative updates of output cells. IE is supported for 5200 programs now.
* Store default values in `setup.txt`

## v22.2.17

### Features

* Universal macOS binary for Intel (x86-64) and Apple M1 processors (ARM64)

## v22.2.15

### Features

* Statically linked binaries for Linux:
  * `loda-linux-x86`: for Intel/AMD x86. Replaces dynamically linked Ubuntu binaries.
  * `loda-linux-arm64`: for ARM64 processors. Compatible with Raspberry Pi OS (64-bit).

### Bugfixes

* Fix two issues during update installation on Windows
* Fix program submission error (#106)

## v22.2.10

### Bugfixes

* Fix profile name in submitted programs

## v22.2.8

### Features

* New metric: number of received programs per miner profile
* Automatically replace executable during updates on Windows

### Enhancements

* Optimizer supports partial evaluation inside of loops
* Incremental evaluation (IE) partially supports programs with `seq` operations (IE supports 2300 programs now)
* Don't use IE in maintenance mode (to detect bugs)

## v22.1.25

### Bugfixes

* Fix race condition: `curl: (23) Failed writing body` when fetching session ID from API server
* Fix suppressed debug logs for `eval` command

### Features

* Improved performance of simple loops using incremental evaluation (IE). Uses static code analysis to decide whether IE can be applied for a given program. Currently, about 1370 programs fulfill these reqirements. Benchmark of selected programs:

| Sequence | Terms  | Reg Eval | Inc Eval |
|----------|--------|----------|----------|
| A057552  | 300    | 5.11s    | 0.05s    |
| A079309  | 300    | 5.08s    | 0.05s    |
| A012866  | 1000   | 2.80s    | 0.01s    |
| A000045  | 2000   | 6.13s    | 0.01s    |
| A130487  | 5000   | 8.16s    | 0.01s    |

## v22.1.16

### Bugfixes

* Extend timeout for folder locking on Windows

### Features

* Retrieve OEIS files from cache on API server

### Enhancements

* Improve setup: ask for usage statistics and clean up existing settings.

## v22.1.9

### Bugfixes

* Escape characters in generated program lists (Markdown format)

### Features

* Built-in parallel mining command: `loda mine -p` (replaces `mine_parallel.sh` script)
* New metrics for counting program submissions and miner activity

### Enhancements

* Boost performance of program list generation

## v22.1.3

### Enhancements

* Improve mining performance by ignoring duplicate matches
* Improve performance of server-side processing of submitted programs
* Integrate program maintenance into server-side miner loop (removes `maintain` command)

## v21.12.30

### Enhancements

* Reduce memory consumption of generator v1
* Improve scheduling of multi-generator
* Improve cache handling of interpreter and matcher
* Less mining in server mode (focus on submitted programs)

## v21.12.23

### Bugfixes

* Fix optimizer and minimizer bugs involving indirect memory access

### Features

* Automatic clean up of local programs in client mode (not on Ubuntu 18)
* Configurable mutation rate for generator v6
* New metric for removed programs

### Enhancements

* Remove broken generator and matcher metrics
* Prefer latest programs for mutations
* Improve usability of setup

## v21.12.15

### Bugfixes

* Fix program submission error

## v21.12.13

### Enhancements

* Improve scheduling and logging
* Improve error handling on API client errors

## v21.12.1

### Bugfixes

* Fix `git` commands if there are spaces in the path

### Features

* Adjust semantics of conditional division: `dif(x,0) = x`

### Enhancements

* Retry program submissions on connection errors
* Improve Slack and Twitter error handling
* Improve logging when stats generation is slow
* Remove Windows return characters from comments

## v21.11.25

### Features

* Windows support (no `mine_parallel.sh` equivalent yet)

### Enhancements

* Reduce metrics publish interval to reduce load on InfluxDB
* Setup supports configuration in `zsh` environment (macOS)

## v21.11.15

### Bugfixes

* Fix race condition during program submission

### Features

* Setup asks for update interval for OEIS files and programs repository

### Enhancements

* Limit the length of the terms comment in generated programs
* Generator v1 also uses mutator (if templates are defined)
* Clean up fetched program files in server mode
* Avoid programs with loops with constant number of iterations

## v21.11.9

### Enhancements

* Define `gcd(0,0) = 0` instead of infinity
* Check for updates and remind user via log message
* Submit minimized and formatted programs in client mode
* Optimize server-side processing of submitted programs
* Improve decision for "better" programs
* Improve scheduling and event logging
* Extend big number range of b-file sequence terms

## v21.10.26

### Features

* Support custom miner configuration file `miners.json`
* Auto update default miner configuration file `miners.default.json`
* New program generator (v6) which mutates existing programs

### Enhancements

* Improved program fetching in server mode
* Generalized mutations of new programs
* Improve scheduling and event logging

## v21.10.12

### Bugfixes

* Fix crash during update using `setup` command on Ubuntu
* Fix `submit` command: overwrite existing programs if faster/better
* Fix decision on faster/better programs for short sequences
* Fix early overflow in `pow` operation

### Enhancements

* Miner profile `update` uses overwrite mode `all`

## v21.10.8

### Features

* Added `submit` command to check and transfer hand-written programs.

### Bugfixes

* Fix error in `mine_parallel.sh`: loda executable not found

### Enhancements

* Publish programs and sequence metrics after stats loading
* Removed hard-coded checks for special sequences (Collatz)
* Big number format uses 64-bit words: faster and larger range
* Updated default miner configuration
* Enhanced benchmark for comparing releases:

| Sequence | Max Terms | Time for 100 Terms |
|----------|-----------|--------------------|
| A000796  |    316    |      139.2ms       |
| A001113  |    319    |      123.1ms       |
| A002110  |    339    |      112.4ms       |
| A002193  |    119    |      2752.1ms      |

## v21.9.28

### Bugfixes

* Fix start-up error `Error parsing line...` (#36)

### Features

* Add optional "Submitted by ..." comments in mined programs.
* Setup automatically installs and updates `mine_parallel.sh` script and default miner configuration.
* Setup checks for updates and installs them on demand.

### Enhancements

* Optimized server-side processing of submitted programs.
* Regularly update OEIS index and programs folder, and regenerate stats. No more restarts of miner processes needed.
* Benchmark:

| Sequence | Max Terms | Time for 100 Terms |
|----------|-----------|--------------------|
| A000796  |    283    |      153.1ms       |
| A001113  |    286    |      133.9ms       |
| A002110  |    309    |      118.5ms       |
| A002193  |    106    |      3566.6ms      |

## v21.9.21

### Bugfixes

* Fix memory usage statistics on MacOS (Big Sur).

### Features

* Introduced mining modes: local, client and server mode. In the client mode (default), miners automatically submit their findings to a central server. Clients don't need to create pull requests anymore to contribute programs.
* Change maximum memory usage with the setup command.

### Enhancements

* Support for multiple web clients: `curl` (default) and `wget`.
* Migrated environment variable configuration to `loda/setup.txt`.
* Improved messages and usability of the setup command.
* Replace `match` command by extending `mine` with an optional argument.
