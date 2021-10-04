To install or update LODA, please follow the [installation instructions](https://loda-lang.org/install/). Here the release notes:

## [Unreleased]

### Bugfixes

* Fix error in `mine_parallel.sh`: loda executable not found

### Enhancements

* Remove hard-coded Collatz checks

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
