#include "miner.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <unordered_set>

#include "config.hpp"
#include "file.hpp"
#include "generator.hpp"
#include "interpreter.hpp"
#include "log.hpp"
#include "mutator.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"

const std::string Miner::ANONYMOUS("anonymous");
const int64_t Miner::PROGRAMS_TO_FETCH = 2000;  // magic number
const int64_t Miner::NUM_MUTATIONS = 100;       // magic number

Miner::Miner(const Settings &settings, ProgressMonitor *progress_monitor)
    : settings(settings),
      mining_mode(Setup::getMiningMode()),
      validation_mode(ValidationMode::EXTENDED),  // set in reload()
      log_scheduler(36),                          // 1% steps (magic number)
      metrics_scheduler(Metrics::get().publish_interval),
      cpuhours_scheduler(3600),  // 1 hour (fixed!!)
      api_scheduler(300),        // 5 minutes (magic number)
      reload_scheduler(21600),   // 6 hours (magic number)
      progress_monitor(progress_monitor),
      num_processed(0),
      num_removed(0),
      num_reported_hours(0),
      current_fetch(0) {}

void Miner::reload() {
  api_client.reset(new ApiClient());
  manager.reset(new OeisManager(settings));
  manager->load();
  manager->getFinder();  // initializes stats and matchers
  const auto miner_config = ConfigLoader::load(settings);
  profile_name = miner_config.name;
  validation_mode = miner_config.validation_mode;
  if (mining_mode == MINING_MODE_SERVER || ConfigLoader::MAINTAINANCE_MODE) {
    multi_generator.reset();
  } else {
    multi_generator.reset(
        new MultiGenerator(settings, manager->getStats(), true));
  }
  mutator.reset(new Mutator(manager->getStats()));
  manager->releaseStats();  // not needed anymore
}

void Miner::mine() {
  // load manager
  if (!manager) {
    reload();
  }

  if (progress_monitor) {
    // start background thread for progress monitoring
    auto monitor = progress_monitor;
    std::thread thread([monitor]() {
      const auto delay = std::chrono::seconds(36);  // 1% steps (magic number)
      while (!monitor->isTargetReached()) {
        monitor->writeProgress();
        std::this_thread::sleep_for(delay);
      }
      Signals::HALT = true;
    });

    // run main mining loop
    try {
      runMineLoop();
      auto mins = std::to_string(monitor->getElapsedSeconds() / 60);
      Log::get().info("Finished mining after " + mins + " minutes");
    } catch (...) {
      Log::get().error("Caught exception during mining", false);
    }
    try {
      thread.join();
    } catch (...) {
      Log::get().warn("Error joining progress monitoring thread");
    }
  } else {
    // run mining loop w/o monitoring
    runMineLoop();
  }
}

void Miner::runMineLoop() {
  Parser parser;
  std::stack<Program> progs;
  Sequence norm_seq;
  Program program;
  Matcher::seq_programs_t seq_programs;
  update_program_result_t update_result;

  // validate modes
  if (validation_mode == ValidationMode::BASIC &&
      mining_mode == MiningMode::MINING_MODE_CLIENT) {
    Log::get().error("Basic validation not supported in client mining mode",
                     true);
  }

  // print info
  std::string validation_mode_str;
  switch (validation_mode) {
    case ValidationMode::BASIC:
      validation_mode_str = "basic";
      break;
    case ValidationMode::EXTENDED:
      validation_mode_str = "extended";
      break;
  }
  Log::get().info("Mining programs in " + convertMiningModeToStr(mining_mode) +
                  " mode (" + validation_mode_str + " validation mode)");

  std::string submitted_by;
  current_fetch = (mining_mode == MINING_MODE_SERVER) ? PROGRAMS_TO_FETCH : 0;
  num_processed = 0;
  num_removed = 0;
  while (true) {
    // if queue is empty: fetch or generate a new program
    if (progs.empty()) {
      // server mode: try to fetch a program
      if (mining_mode == MINING_MODE_SERVER) {
        if (current_fetch > 0) {
          program = api_client->getNextProgram();
          if (program.ops.empty()) {
            current_fetch = 0;
          } else {
            current_fetch--;
            // check metadata stored in program's comments
            ensureSubmittedBy(program);
            progs.push(program);
          }
        }
      } else {
        // client mode: generate new program
        progs.push(multi_generator->generateProgram());
      }
    }

    if (!progs.empty()) {
      // get the next program
      program = progs.top();
      progs.pop();

      // try to extract A-number from comment (server mode)
      seq_programs.clear();
      auto id = ProgramUtil::getSequenceIdFromProgram(program);
      if (!id.empty()) {
        try {
          OeisSequence seq(id);
          seq_programs.push_back(std::pair<size_t, Program>(seq.id, program));
        } catch (const std::exception &) {
          Log::get().warn("Invalid sequence ID: " + id);
        }
      }

      // otherwise match sequences
      if (seq_programs.empty()) {
        seq_programs = manager->getFinder().findSequence(
            program, norm_seq, manager->getSequences());
      }

      // validate matched programs and update existing programs
      for (auto s : seq_programs) {
        if (!checkRegularTasks()) {
          break;
        }
        program = s.second;
        updateSubmittedBy(program);
        update_result =
            manager->updateProgram(s.first, program, validation_mode);
        if (update_result.updated) {
          // update metrics
          submitted_by = ProgramUtil::getCommentField(
              program, ProgramUtil::PREFIX_SUBMITTED_BY);
          if (submitted_by.empty()) {
            submitted_by = "unknown";
          }
          if (update_result.is_new) {
            num_new_per_user[submitted_by]++;
          } else {
            num_updated_per_user[submitted_by]++;
          }
          // in client mode: submit the program to the API server
          if (mining_mode == MINING_MODE_CLIENT) {
            // add metadata as comments
            program = update_result.program;
            ProgramUtil::addComment(program, ProgramUtil::PREFIX_MINER_PROFILE +
                                                 " " + profile_name);
            ProgramUtil::addComment(program, ProgramUtil::PREFIX_CHANGE_TYPE +
                                                 " " +
                                                 update_result.change_type);
            if (!update_result.is_new) {
              ProgramUtil::addComment(
                  program, ProgramUtil::PREFIX_PREVIOUS_HASH + " " +
                               std::to_string(update_result.previous_hash));
            }
            api_client->postProgram(program, 10);  // magic number
          }
          // mutate successful program
          if (mining_mode != MINING_MODE_SERVER && progs.size() < 1000) {
            mutator->mutateCopies(update_result.program, NUM_MUTATIONS, progs);
          }
        }
      }
    } else {
      // we are in server mode and have no programs to process
      // => lets do maintenance work!
      if (!manager->maintainProgram(mutator->random_program_ids.getFromAll())) {
        num_removed++;
      }
    }

    num_processed++;
    if (!checkRegularTasks()) {
      break;
    }
  }

  // report remaining cpu hours
  while (num_reported_hours < settings.num_mine_hours) {
    reportCPUHour();
  }
}

bool Miner::checkRegularTasks() {
  if (Signals::HALT) {
    return false;  // stop immediately
  }
  bool result = true;

  // regular task: log info
  if (log_scheduler.isTargetReached()) {
    log_scheduler.reset();
    std::string progress;
    if (progress_monitor) {
      const double p = 100.0 * progress_monitor->getProgress();
      std::stringstream buf;
      buf.precision(1);
      buf << ", " << std::fixed << p << "%";
      progress = buf.str();
    }
    if (num_processed) {
      Log::get().info("Processed " + std::to_string(num_processed) +
                      " programs" + progress);
      num_processed = 0;
    } else {
      Log::get().warn("Slow processing of programs" + progress);
    }
  }

  // regular task: fetch programs from API server
  if (mining_mode == MINING_MODE_SERVER && api_scheduler.isTargetReached()) {
    api_scheduler.reset();
    current_fetch += PROGRAMS_TO_FETCH;
  }

  // regular task: publish metrics
  if (metrics_scheduler.isTargetReached()) {
    metrics_scheduler.reset();
    std::vector<Metrics::Entry> entries;
    std::map<std::string, std::string> labels;
    labels["kind"] = "new";
    for (auto it : num_new_per_user) {
      labels["user"] = it.first;
      entries.push_back({"programs", labels, static_cast<double>(it.second)});
    }
    labels["kind"] = "updated";
    for (auto it : num_updated_per_user) {
      labels["user"] = it.first;
      entries.push_back({"programs", labels, static_cast<double>(it.second)});
    }
    labels.clear();
    labels["kind"] = "removed";
    entries.push_back({"programs", labels, static_cast<double>(num_removed)});
    Metrics::get().write(entries);
    num_new_per_user.clear();
    num_updated_per_user.clear();
    num_removed = 0;
  }

  // regular task: report CPU hours
  if (cpuhours_scheduler.isTargetReached()) {
    cpuhours_scheduler.reset();
    reportCPUHour();
  }

  // regular task: reload oeis manager and generators
  if (reload_scheduler.isTargetReached()) {
    reload_scheduler.reset();
    reload();
  }

  return result;
}

void Miner::reportCPUHour() {
  if (Setup::shouldReportCPUHours() && settings.report_cpu_hours) {
    api_client->postCPUHour();
  }
  num_reported_hours++;
}

void Miner::submit(const std::string &path, std::string id) {
  Parser parser;
  auto program = parser.parse(path);
  ConfigLoader::MAINTAINANCE_MODE = true;
  reload();
  if (id.empty()) {
    id = ProgramUtil::getSequenceIdFromProgram(program);
  }
  OeisSequence seq(id);
  Settings settings(this->settings);
  settings.print_as_b_file = false;
  Log::get().info("Validating program for " + seq.id_str());
  Evaluator evaluator(settings);
  auto terms = seq.getTerms(OeisSequence::FULL_SEQ_LENGTH);
  auto result =
      evaluator.check(program, terms, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
  if (result.first == status_t::ERROR) {
    Log::get().error("Validation failed", false);
    settings.print_as_b_file = true;
    Evaluator evaluator2(settings);
    evaluator2.check(program, terms, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
    return;  // error
  }
  Log::get().info("Validation successful");
  // match sequences
  Sequence norm_seq;
  const auto mode = Setup::getMiningMode();
  auto seq_programs = manager->getFinder().findSequence(
      program, norm_seq, manager->getSequences());
  size_t matches = 0;
  for (auto s : seq_programs) {
    program = s.second;
    updateSubmittedBy(program);
    auto r = manager->updateProgram(s.first, program, ValidationMode::EXTENDED);
    if (r.updated) {
      // in client mode: submit the program to the API server
      if (mode == MINING_MODE_CLIENT) {
        // add metadata as comment
        program = r.program;
        ProgramUtil::addComment(program,
                                ProgramUtil::PREFIX_MINER_PROFILE + " manual");
        api_client->postProgram(program);
      }
      matches++;
    }
  }
  if (matches > 0) {
    if (mode == MINING_MODE_LOCAL) {
      Log::get().info("Stored programs for " + std::to_string(matches) +
                      " matched sequences in local programs directory");
      Log::get().warn("Skipping submission to server due to local mode");
    } else {
      Log::get().info("Submitted programs for " + std::to_string(matches) +
                      " matched sequences");
    }
  } else {
    Log::get().info(
        "Skipped submission because there exists already a (faster) program");
  }
}

void Miner::ensureSubmittedBy(Program &program) {
  auto submitted_by =
      ProgramUtil::getCommentField(program, ProgramUtil::PREFIX_SUBMITTED_BY);
  if (submitted_by.empty()) {
    ProgramUtil::addComment(program,
                            ProgramUtil::PREFIX_SUBMITTED_BY + " " + ANONYMOUS);
  }
}

void Miner::updateSubmittedBy(Program &program) {
  auto submitted_by =
      ProgramUtil::getCommentField(program, ProgramUtil::PREFIX_SUBMITTED_BY);
  if (submitted_by.empty()) {
    submitted_by = Setup::getSubmittedBy();
    if (!submitted_by.empty()) {
      ProgramUtil::addComment(
          program, ProgramUtil::PREFIX_SUBMITTED_BY + " " + submitted_by);
    }
  } else if (submitted_by == ANONYMOUS) {
    ProgramUtil::removeCommentField(program, ProgramUtil::PREFIX_SUBMITTED_BY);
  }
}
