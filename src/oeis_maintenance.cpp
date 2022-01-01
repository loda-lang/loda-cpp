#include "oeis_maintenance.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

#include "file.hpp"
#include "oeis_list.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "stats.hpp"
#include "util.hpp"

OeisMaintenance::OeisMaintenance(const Settings &settings)
    : evaluator(settings),
      minimizer(settings),
      manager(settings, true)  // we need to set the overwrite-flag here!
{}

void OeisMaintenance::maintain() {
  // load sequence data
  manager.load();

  // update stats
  manager.getStats();

  // check and minimize programs
  checkAndMinimizePrograms();

  Log::get().info("Finished maintenance of programs");
}

size_t OeisMaintenance::checkAndMinimizePrograms() {
  // only in server mode
  if (Setup::getMiningMode() != MINING_MODE_SERVER) {
    Log::get().warn("Skipping program maintenance because not in server mode");
    return 0;
  }

  Log::get().info("Checking and minimizing programs");
  size_t num_processed = 0, last_processed = 0, num_minimized = 0,
         num_removed = 0, last_removed = 0;
  Parser parser;
  Program program, minimized;
  std::string file_name, submitted_by;
  bool is_okay, is_protected;
  AdaptiveScheduler log_scheduler(600);  // every 10 minutes; magic number

  // generate random order of sequences
  std::vector<size_t> ids;
  ids.resize(manager.sequences.size());
  const size_t l = ids.size();
  for (size_t i = 0; i < l; i++) {
    ids[i] = i;
  }
  auto rng = std::default_random_engine{};
  rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
  std::shuffle(std::begin(ids), std::end(ids), rng);

  // check programs for all sequences
  for (auto id : ids) {
    auto &s = manager.sequences[id];
    if (s.id == 0) {
      continue;
    }
    file_name = s.getProgramPath();
    std::ifstream program_file(file_name);
    if (program_file.good()) {
      Log::get().debug("Checking program for " + s.to_string());
      try {
        program = parser.parse(program_file);
        submitted_by = ProgramUtil::getSubmittedBy(program);
        auto extended_seq = s.getTerms(OeisSequence::FULL_SEQ_LENGTH);

        // check its correctness
        auto check = evaluator.check(program, extended_seq,
                                     OeisSequence::DEFAULT_SEQ_LENGTH, id);
        is_okay = (check.first != status_t::ERROR);  // we allow warnings

        // check if it is on the deny list
        if (manager.deny_list.find(s.id) != manager.deny_list.end()) {
          is_okay = false;
        }
      } catch (const std::exception &exc) {
        std::string what(exc.what());
        if (what.rfind("Error fetching", 0) ==
            0)  // user probably hit ctrl-c => exit
        {
          break;
        }
        Log::get().error(
            "Error checking " + file_name + ": " + std::string(exc.what()),
            false);
        is_okay = false;
        continue;
      }

      if (!is_okay) {
        // send alert and remove file
        manager.alert(program, id, "Removed invalid", "danger", "");
        program_file.close();
        remove(file_name.c_str());
        num_removed++;
        last_removed++;
      } else {
        is_protected = false;
        if (manager.protect_list.find(s.id) != manager.protect_list.end()) {
          is_protected = true;
        }
        if (!is_protected && !ProgramUtil::isCodedManually(program)) {
          ProgramUtil::removeOps(program, Operation::Type::NOP);
          minimized = program;
          minimizer.optimizeAndMinimize(minimized, 2, 1,
                                        OeisSequence::DEFAULT_SEQ_LENGTH);
          if (program != minimized) {
            // manager.alert( minimized, id, "Minimized", "warning" );
            num_minimized++;
          }
          manager.dumpProgram(s.id, minimized, file_name, submitted_by);
        }
      }
      num_processed++;
      last_processed++;

      // regular task: log info and publish metrics
      if (log_scheduler.isTargetReached()) {
        log_scheduler.reset();
        Log::get().info("Processed " + std::to_string(last_processed) +
                        " programs");
        std::vector<Metrics::Entry> entries;
        std::map<std::string, std::string> labels;
        labels["kind"] = "removed";
        entries.push_back(
            {"programs", labels, static_cast<double>(last_removed)});
        Metrics::get().write(entries);
        last_processed = 0;
        last_removed = 0;
      }
    }
  }

  if (num_removed > 0) {
    Log::get().info("Removed " + std::to_string(num_removed) +
                    " invalid programs in total");
  }
  if (num_minimized > 0) {
    Log::get().info("Minimized " + std::to_string(num_minimized) + "/" +
                    std::to_string(num_processed) + " programs in total");
  }

  return num_removed + num_minimized;
}
