## [Unreleased]

### Features

* Introduced mining modes: local, client and server mode. In the client mode (default), miners automatically submit their findings to a central server. Clients don't need to create pull-requests anymore to contribute programs.

### Enhancements

* Support for multiple web clients: `curl` (default) and `wget`.
* Migrated environment variable configuration to `loda/setup.txt`.
* Improved messages and usability of the setup command.
