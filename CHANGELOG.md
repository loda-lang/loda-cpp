## [Unreleased]

* Improve `lex` range computation
* Add term limit checks in formula generation to prevent exponential complexity growth (e.g., A042269)

### Bugfixes

* Fix Windows `upgrade` command to download .zip files instead of .exe
* Improve error messages to show HTTP status codes for failed downloads

## v26.2.14

### Enhancements

* Support `dif`, `dir` and `lex` in formula generation
* Support `bin` (binomial coefficient) in LEAN formula generation
* Improve `nrt` range computation
* Enhance `check` command to verify program offset
* Add `export-formulas` to release build

## v26.1.25

### Enhancements

* Optimize formula generation performance
* Increase default maximum physical memory and print usage

### Bugfixes

* Fix LEAN type mismatch for nested sequence calls (A111432)

## v26.1.16

### Bugfixes

* Fix JSON serialization

## v26.1.15

### Enhancements

* Improve JSON serialization and API request handling
* Improve `upgrade` command output

### Bugfixes

* Fix integer overflow in optimizer
* Fix invalid DLL on windows-arm64

## v26.1.13

### Bugfixes

* Fix calculation of number of needed initial terms (A131083)
* Fix `test-lean` failure when LEAN project directory exists but is incomplete
* Fix LEAN type synthesis error by wrapping pattern offset constants with `Int.ofNat` in Nat domain formulas
* Fix LEAN export incorrectly applying pattern offset to `Int` domain parameters (A046231)
* Fix PARI error handling and test evaluation
* Fix stack overflow in formula evaluators for sequences with power operations where exponent can be negative (A132076)

### Enhancements

* Add optimizer case for `nrt`
* Add internal command `test-recursion`
* Add internal command `update-formula-tests` to automatically update formula test expectations
* Add program change log in server mode to track program updates
* Use v2 API endpoints to download OEIS data and report stats
* Set user agent in web requests

## v25.12.1

### Enhancements

* Integrate `libcurl` for HTTP requests, replacing external `curl`/`wget` tools
* Integrate `zlib` for gzip decompression, replacing external `gzip`/`gunzip` tools
* Report broken b-files to LODA API server for automatic cache invalidation
* Add internal `export-formulas` command to export all program formulas to a file or stdout

### Bugfixes

* Fix PARI evaluation error for formulas with constant identity functions (e.g., A026765)
* Fix LEAN evaluation error for formulas with mutually recursive functions that are not self-recursive (e.g., A001636)

## v25.11.29

### Enhancements

* Improve formula generation performance by imposing tighter limits on variant simplification
* Improve operations merging in optimizer

### Bugfixes

* Fix parsing error during stats generation
* Fix PARI formula generation for rising factorial when the base can be zero (e.g., A006430)

### Enhancements

* Use `v2/submissions/` API endpoint to fetch and submit programs
* Handle program submissions with `delete` mode

## v25.11.9

### Enhancements

* Support `fac` in LEAN formula generation
* Improve LEAN formula generation for `pow`
* Improve mined programs comparison

## v25.11.2

### Bugfixes

* Fix escaping in generated Discord messages

### Features

* Support multiple functions in LEAN formula generation
* Support bitwise operations in LEAN formula generation

### Enhancements

* Simplify geometric progressions in formula generation
* Simplify generated expressions for `fac` operations
* Improve optimization of `pow`,`nrt` operations
* Extend program stats with LEAN & PARI flags
* Add LEAN project support for testing

## v25.10.19

### Bugfixes

* Fix optimizer bug

### Features

* Support memory operations in formula generation
* Support simple recursive functions in LEAN formula generation
* Add Windows ARM64 build support

### Enhancements

* Add formula parser for parsing formulas from their string representation
* Add internal command for finding programs with slow formula generation
* Improve eval performance using optimized linker settings
* Extend program stats with operation type bitmask

## v25.10.16

### Bugfixes

* Fix memory region operation handling in incremental evaluator

## v25.10.15

### Features

* Add memory operations: `fil`, `rol`, `ror`
* Add limited LEAN export for simple programs

### Enhancements

* Simplify formulas by replacing simple references to recursive functions
* Removed deprecated list generation functionality

## v25.10.8

### Enhancements

* Improve formula generation
* Improve submitted program checks
* Avoid cloning entire `git` history for programs
* Add internal commands and tools

## v25.10.3

### Bugfixes

* Fix `nrt` operation

## Enhancements

* Support `fac` in formula generation
* Extend program stats

## v25.9.26

### Enhancements

* Faster `nrt` using Newton's method
* Correct existing programs
* Extend program stats

## v25.9.13

### Enhancements

* Extend program stats
* Improve error output in `eval` command

## v25.9.10

### Enhancements

* Improve program validation
* Improve usage stats
* Allow submissions w/o miner profile

## v25.9.7

### Enhancements

* Extensions to run on API server

## v25.9.4

### Enhancements

* Generate submitter stats

## v25.8.30

### Bugfixes

* Fix `move` command on Windows
* Fix optimizer bug

## v25.8.28

### Bugfixes

* Fix missing sequence name error

## v25.8.24

### Bugfixes

* Fix optimizer crash on Windows

### Enhancements

* Reorganize folder structure
* Improve check for faster programs
* Improve logs

## v25.8.9

### Enhancements

* Improve performance of embedded sequence programs (virtual evaluation)

## v25.8.4

### Bugfixes

* Fix hanging `fac` operation
* Fix range computation of `fac` operation

* Improve check for simpler programs

## v25.7.24

### Features

* Added `fac` operation for falling/rising factorial

# Enhancements

* Use two's-complement representation for bitwise operations

## v25.7.14

### Bugfixes

* Fix stuck formula generation
* Fix incorrect check

## v25.7.13

### Bugfixes

* Fix segmentation fault during mining
* Fix `upgrade` command

## v25.7.12

### Bugfixes

* Fix range computation for `dgr`
* Fix IE bug occuring when using step count > 1 and `max` on loop counter
* Fix argument check for `seq` for programs with offset
* Fix minimization/optimization loop
* Fix program submission bug

### Enhancements

* Check range in program validation

## v25.6.27

### Bugfixes

* Fix overflow in formula generator

### Features

* Added `upgrade` command and speed up `update`
* Added `range` output format for export
* Support bitwise operators with negative values in formula generation
* Support binomial with negative args in PARI code generation

### Enhancements

* Add PARI evaluation timeout
* Remove `srt` operation
* Limit maximum number of initial terms in formula generation
* Improve maintenance checks

## v25.5.24

### Bugfixes

* Fix in formula generation
* Fix in optimizer

### Features

* Support comparison operators in formula generation
* Support `dgr` and `dgs` operations in formula generation
* Support factorial in formula generation

### Enhancements

* Internal command to add sequences to lists such as `protect.txt`

## v25.5.18

### Enhancements

* Do not prefer log eval program

## v25.5.8

### Enhancements

* Improved log messages of `submit` command

## v25.3.29

### Features

* Added `dir` operation for repeated division

## v25.2.15

### Bugfixes

* Fix duplicate stats regeneration
* Fix sporadic `git pull` error

## v25.1.31

### Enhancements

* Use SSL connection to API server

## v25.1.8

### Enhancements

* Rename digit operations
* Improve offset handling
* Improve optimizer

## v25.1.3

### Bugfixes

* Fail on recently corrupted programs directory in BOINC

## v24.12.28

### Bugfixes

* Fix empty programs directory error in BOINC

## v24.12.27

### Enhancements

* Improve program unfolding and maintenance

## v24.12.16

### Bugfixes

* Fix error handling

## v24.12.15

### Enhancements

* Improve negativity check in formula generation
* Simplify formulas generated from `nrt` operations
* Limited support for bit-wise operations in formulas
* Delete corrupt programs directory in BOINC

## v24.12.8

### Enhancements

* Separate binaries for MacOS arm64
* Support `log` and `nrt` operation in formula generation
* Internal command to clean up unused `mod` instructions

## v24.11.29

### Features

* Support for sequence offsets (extracted from the OEIS)

## v24.10.20

### Features

* Added `lex` operation (largest exponent)

## v24.9.15

### Bugfixes

* Fix optimizer bugs
* Fix program folding bugs

## v24.9.7

### Bugfixes

* Fix optimizer `prg` handling
* Fix max memory option `-m`
* Fix program folding

### Enhancements

* Support `prg` in `unfold` command
* Internal `replace` command
* Remove deprecated dot export
* Collapse `mov` loops in optimizer

## v24.9.2

## Featues

* Support for directives and `prg` instruction

### Enhancements

* Incremental evaluator supports `max` on loop counter

## v24.8.23

### Bugfixes

* Fix hanging process on initialization errors

### Features

* Send miner alerts to Discord channel

## v24.8.19

### Enhancements

* Improve BOINC progress monitoring
* Improve logs for b-file handling
* Optimizer: replace double `equ` by `neq`

## v24.8.18

### Bugfixes

* Remove invalid b-files from cache

## v24.8.15

### Bugfixes

* Fix default OEIS fetch setting

## v24.8.14

### Features

* Add comparison operations: `equ` (formerly `cmp`),`neq`,`leq`,`geq`
* Add bitwise operations: `ban`, `bor`, `bxo`
* Add digit operations: `dis` (digit sum), `dir` (digital root)
* Add region operations: `clr` (clear), `srt` (sort)
* Add `nrt` (n-th root) and `log` (logarithm) operations

### Enhancements

* Simplify generated formulas

## v24.1.7

### Bugfixes

* Fix memory leak in expression util

### Enhancements

* Use floor/truncate functions in plain formulas
* Simplify formulas using Gaussian elimination
* Simplify sums and products in formulas
* Extended internal maintenance command
* Use more terms during minimization

## v23.11.19

### Features

* Support fetching resources directly from OEIS

## v23.10.8

### Bugfixes

* Fix errors during loading and saving of stats

## v23.9.30

### Features

* Added `fold` command to replace a subprogram by a `seq` operation

### Enhancements

* Incremental evaluator and formula generator support more programs
* Optimizer improvements

## v23.8.27

### Enhancements

* Improve miner preferences regarding large constants
* Optimizer enhancements

## v23.8.19

### Enhancements

* Improve miner preferences for better programs

## v23.8.12

### Bugfixes

* Disable reduced number of terms for exponential programs

## v23.8.6

### Enhancements

* Detect programs with exponential complexity
* Remove Twitter support

## v23.7.9

### Enhancements

* Support formulas when loop counter is not `$0`
* Reduce minimum number of terms to 80
* Increase maximum number of steps to 100 million

## v23.7.3

### Enhancements

* Formula generation supports pre-loop transformations and loop counter decrements >1
* Improved check for logarithmic program complexity

## v23.6.23

### Enhancements

* Remove deprecated `clr` operation
* Require miner profile in submitted programs
* Stricter check for logarithmic program complexity

## v23.6.17

### Enhancements

* Incremental evaluation supports loop counter decrements >1
* Optimizer removes "commutative detours"

## v23.6.3

### Bugfixes

* Fix hanging process (infinite loop)

## v23.6.2

### Bugfixes

* Detect optimization/minimization loops

### Enhancements

* Auto-unfold `seq` operations

## v23.5.6

### Features

* Added option `-z` to limit the maximum evaluation time

### Enhancements

* The `submit` command considers `-i` option for better control and performance

## v23.5.1

### Enhancements

* Improve output and behavior of generator v8 (reading programs from batch file)
* Improve decision for better/faster programs

## v23.4.30

### Bugfixes

* Fix decision making for simpler/faster programs

### Enhancements

* Increase max memory and max number of steps

## v23.4.28

### Bugfixes

* Fix implicitly initialized cells in `unfold` command

### Enhancements

* Improve reduction of constants in optimizer
* Improve decision for faster vs. simpler programs
* Retain `seq` comments in `unfold` command

### Features

* Mining by reading programs from a batch file (generator v8)

## v23.2.18

### Enhancements

* Faster `pow` operation
* Increase maximum number of steps to 60 million
* Extend maximum number of decimal digits to 1155
* Small enhancements in program mutator

## v23.1.25

### Features

* Added `unfold` command to embed called seqeunce programs

### Enhancements

* Reduce default number of terms to 8 (include 10k more OEIS seqs)
* Selective auto-unfolding during program maintenance
* Prefer faster over IE for programs that are used a lot

## v23.1.11

### Enhancements

* Improve decision for better programs

## v23.1.5

### Bugfixes

* Fix integer overflow

## v23.1.4

### Enhancements

* Improve interpreter performance
* Increase maximum step count to 50 million
* Support more recursive formulas
* Metric for number of formulas
* Use local variables in generated PARI code
* Prefer IE programs with trivial post-loop code
* Improve log messages for processed programs

## v22.12.14

### Bugfixes

* Fix validation for IE programs
* Use transitive hash values for programs with `seq`

### Enhancements

* Add detection of programs with logarithmic complexity
* Add stats for incremental and logarithmic eval

## v22.12.9

### Bugfixes

* Fix decision making for better/faster programs

## v22.12.8

### Enhancements

* Reduce server-side validation effort
* Introduce threshold for faster/better programs (5%)

## v22.12.6

### Bugfixes

* Fix vector exception

### Enhancements

* Increase maximum number of cycles to 30 million

## v22.12.2

### Enhancements

* Increase default OEIS update interval

## v22.12.1

### Bugfixes

* Fix update issues

### Features

* Install script
* Update command

### Enhancements

* Support more programs in incremental evaluation
* Increase maximum number of cycles to 25 million
* Tweet only for first programs

## v22.11.22

### Bugfixes

* Fix incorrect result of incremental evaluator

### Enhancements

* Support more recursive formulas
* Internal command for finding slow programs

## v22.11.12

### Bugfixes

* Fix step count inconsistency
* Check maximum number of steps in incremental evaluator
* Avoid fixed number of 100 terms in `minimize` command; now configurable using `-t` with default of 10 terms

### Features

* Support `seq` operation in formula generator

## v22.11.7

### Bugfixes

* Fix insecure fall-back option for `curl`

### Enhancements

* Support more recursive in formulas

### Features

* Support disabling of profiles

## v22.10.28

### Bugfixes

* Fix step count calculation
* Fix incremental evaluation
* Fix expression normalization

### Features

* Support `bin`, `gcd` and `trn` operations in formula generator

### Enhancements

* Increase max interpreter cycles to 20 million
* Support incremental evaluation for more programs (from 9k to 15k programs)
* Activate incremental evaluation in validation

## v22.10.19

### Enhancements

* Support `mod`, `min` and `max` in formulas
* Support recursive formulas for a small subset of IE programs
* Use `floor` instead of `truncate` in PARI if possible
* Add insecure fall-back option for `curl`

## v22.10.16

### Bugfixes

* Fix programs update interval

### Enhancements

* Support `div` in formulas

## v22.10.14

### Enhancements

* Separate update intervals for OEIS files (7 days) and programs repository (1 day)

## v22.10.13

### Features

* New `export` command for converting programs to [PARI/GP](https://pari.math.u-bordeaux.fr/)
* Add formulae to programs

### Enhancements

* Optimizer collapses simple loops

## v22.9.25

### Bugfixes

* Forbid invalid number formats in parser

## v22.9.17

### Enhancements

* Forbid indirect memory access with negative indices
* Log total system memory in BOINC mode

## v22.9.12

### Bugfixes

* Additional check to avoid duplicate program updates

### Enhancements

* Internal `compare` command for checking if a program is faster/better
* Ignore `seq` arguments in constants stats
* Update BOINC progress after `git clone`

## v22.9.8

### Bugfixes

* Fix programs directory check

### Enhancements

* Optimizer merges repeated `add` and `mul` operations
* Remove sequences from deny list in `no_loda.txt`
* Increase default max memory cells to 1000 (for [A000041](https://oeis.org/A000041))

## v22.8.21

### Enhancements

* Minimizer tries to replace loops with constant number or iterations
* Include LODA version in hash of transmitted programs
* Improve decision for "faster" programs
* Internal command for generating program lists

## v22.8.12

### Features

* Added `mutate` command

### Enhancements

* Increase max number of interpreter cycles to 15 million
* Improve decision for "better" programs
* Clear caches before comparing programs

## v22.8.4

### Bugfixes

* Fix race condition when creating directories
* Fix detection of recursive calls

### Enhancements

* Delete invalid matches cache based on BOINC input flag

## v22.7.29

### Bugfixes

* Fix embedded `git` on Windows

## v22.7.28

### Bugfixes

* Fix CPU hour reporting using `wget`

### Features

* Embedded `git` for BOINC on Windows
* Read parameters from BOINC task input

### Enhancements

* Increase max number of interpreter cycles to 10 million

## v22.7.23

### Bugfixes

* Fix arithmetic exception
* Fix segmentation fault due to static linking problem

### Enhancements

* Reduce max number of terms to 1000
* Print debug log message if CPU hour reporting fails
* Catch errors of progress monitoring thread

## v22.7.22

### Bugfixes

* Interrupt long-running evaluations if time limit is reached
* Fix handling of protected programs
* Fix progress monitor for native mining (non-BOINC)
* Fix escaping in Slack messages and metric labels

### Enhancements

* Include progress information in logs
* Simplify comparison of programs (better/faster)
* Force stats regeneration after OEIS and programs update

## v22.6.27

### Bugfixes

* Fix error message in `check` command

### Enhancements

* Improve handling of loops with constant number of iterations
* Improve detection of fake "better" programs
* Disable incremental evaluation in `check` command for consistency
* Adjust maximum big int size to handle A000336

## v22.6.19

### Features

* Add username to program count metrics
* Log `hostid` in BOINC mode

### Enhancements

* Improve server-side program validation
* Reduce update interval to 1 day

## v22.6.12

### Bugfixes

* Fix issue where tasks are stuck at 100% progress

### Features

* Support `full_check.txt` list for sequences that require checking up to 100k terms

## v22.6.6

### Bugfixes

* Fix basic validation for updated programs

### Features

* Configurable API server

### Enhancements

* Skip unnecessary sequence matching in server mode

## v22.5.31

### Enhancements

* Support for reduced ("basic") validation on the server
* Reduce file size of executable on Linux and MacOS

## v22.5.23

### Bugfixes

* Extend folder locking time out value on Windows
* Remove fall-back download from OEIS

### Features

* Checkpoint support for BOINC

### Enhancements

* Change default update interval to 3 days
* Throttling and exponential backoff for API server downloads

## v22.5.20

### Bugfixes

* Fix "access denied" error during stats generation on Windows.

## v22.5.14

### Bugfixes

* Use project dir as fall-back for tmp dir

## v22.5.13

### Enhancements

* Debug logs for BOINC

## v22.5.12

### Bugfixes

* Fix various errors in BOINC on Windows

## v22.5.4

### Enhancements

* Improve progress monitoring in BOINC

## v22.5.3

### Bugfixes

* Fixes setup issues when runing in BOINC

## v22.5.2

### Enhancements

* Use shared project directory when running in BOINC
* Extract user name from BOINC metadata
* Report progress to BOINC wrapper app

## v22.5.1

### Features

* Added option `-H` for settings the number of mining hours (needed for BOINC)
* Internal `boinc` command

## v22.4.17

### Enhancements

* Use 50% of the new and 50% of the last modified programs for mutations

## v22.4.10

### Features

* Added `profile` command for measuring program evaluation time
* Added pattern-based generator (generator: `v7`, miner profile: `pattern`)

### Enhancements

* Improve handling of programs with minimization errors/warnings
* Improve comparsion of programs supporting IE
* Improve algebraic program optimizations
* Avoid duplicate evaluation when updating programs

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
