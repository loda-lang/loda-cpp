To install or update LODA, please follow the [installation instructions](https://loda-lang.org/install/). Here are the release notes:

## [Unreleased]

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
