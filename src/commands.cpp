#include "commands.hpp"

#include <iostream>

#include "test.hpp"
#include "util.hpp"

void Commands::help() {
  Settings settings;
  std::cout << "Usage:                   loda <command> <options>" << std::endl;
  std::cout << std::endl << "=== Core commands ===" << std::endl;
  std::cout << "  evaluate <file/seq-id> Evaluate a program to a sequence (cf. "
               "-t,-b,-s)"
            << std::endl;
  std::cout
      << "  minimize <file>        Minimize a program and print it (cf. -t)"
      << std::endl;
  std::cout << "  optimize <file>        Optimize a program and print it"
            << std::endl;
  std::cout << "  generate               Generate a random program and print it"
            << std::endl;
  std::cout << std::endl << "=== OEIS commands ===" << std::endl;
  std::cout
      << "  mine                   Mine programs for OEIS sequences (cf. -i)"
      << std::endl;
  std::cout
      << "  match <file>           Match a program to OEIS sequences (cf. -i)"
      << std::endl;
  std::cout << "  check <seq-id>         Check a program for an OEIS sequence "
               "(cf. -b)"
            << std::endl;
  std::cout
      << "  maintain               Maintain all programs for OEIS sequences"
      << std::endl;
  std::cout << std::endl << "=== Command-line options ===" << std::endl;
  std::cout << "  -t <number>            Number of sequence terms (default: "
            << settings.num_terms << ")" << std::endl;
  std::cout << "  -b <number>            Print result in b-file format from a "
               "given offset"
            << std::endl;
  std::cout << "  -s                     Evaluate program to number of "
               "execution steps"
            << std::endl;
  std::cout << "  -c <number>            Maximum number of interpreter cycles "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -m <number>            Maximum number of used memory cells "
               "(no limit: -1)"
            << std::endl;
  std::cout
      << "  -p <number>            Maximum physical memory in MB (default: "
      << settings.max_physical_memory / (1024 * 1024) << ")" << std::endl;
  std::cout << "  -l <string>            Log level (values: "
               "debug,info,warn,error,alert)"
            << std::endl;
  std::cout
      << "  -k <string>            Configuration file (default: loda.json)"
      << std::endl;
  std::cout
      << "  -i <string>            Name of miner configuration from loda.json"
      << std::endl;
  std::cout << std::endl << "=== Environment variables ===" << std::endl;
  std::cout << "LODA_PROGRAMS_HOME       Directory for mined programs "
               "(default: programs)"
            << std::endl;
  std::cout << "LODA_UPDATE_INTERVAL     Update interval for OEIS metadata in "
               "days (default: " +
                   std::to_string(settings.update_interval_in_days) + ")"
            << std::endl;
  std::cout << "LODA_MAX_CYCLES          Maximum number of interpreter cycles "
               "(same as -c)"
            << std::endl;
  std::cout << "LODA_MAX_MEMORY          Maximum number of used memory cells "
               "(same as -m)"
            << std::endl;
  std::cout
      << "LODA_MAX_PHYSICAL_MEMORY Maximum physical memory in MB (same as -p)"
      << std::endl;
  std::cout
      << "LODA_SLACK_ALERTS        Enable alerts on Slack (default: false)"
      << std::endl;
  std::cout
      << "LODA_TWEET_ALERTS        Enable alerts on Twitter (default: false)"
      << std::endl;
  std::cout << "LODA_INFLUXDB_HOST       InfluxDB host name (URL) for "
               "publishing metrics"
            << std::endl;
  std::cout << "LODA_INFLUXDB_AUTH       InfluxDB authentication info "
               "('user:password' format)"
            << std::endl;
}

void Commands::test() {
  Test test;
  test.all();
}
