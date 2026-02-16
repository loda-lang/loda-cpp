#include "mine/miner.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <unordered_set>

#include "eval/interpreter.hpp"
#include "eval/optimizer.hpp"
#include "gen/generator.hpp"
#include "lang/comments.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "mine/config.hpp"
#include "mine/mine_manager.hpp"
#include "mine/mutator.hpp"
#include "seq/seq_program.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/metrics.hpp"

const std::string Miner::UNKNOWN("unknown");
const int64_t Miner::PROGRAMS_TO_FETCH = 2000;  // magic number
const int64_t Miner::MAX_BACKLOG = 1000;        // magic number
const int64_t Miner::NUM_MUTATIONS = 100;       // magic number

Miner::Miner(const Settings& settings, ProgressMonitor* progress_monitor)
    : settings(settings),
      mining_mode(Setup::getMiningMode()),
      validation_mode(ValidationMode::EXTENDED),  // set in reload()
      submit_mode(false),
      log_scheduler(36),  // 36 seconds => 1% steps
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
  manager.reset(new MineManager(settings));
  manager->load();
  manager->getFinder();  // initializes stats and matchers
  const auto miner_config = ConfigLoader::load(settings);
  profile_name = miner_config.name;
  validation_mode = miner_config.validation_mode;
  if (mining_mode == MINING_MODE_SERVER || submit_mode) {
    multi_generator.reset();
  } else {
    if (!multi_generator || multi_generator->supportsRestart()) {
      multi_generator.reset(new MultiGenerator(settings, manager->getStats()));
    }
  }
  mutator.reset(new Mutator(manager->getStats()));
}

void signalShutdown() {
  if (!Signals::HALT) {
    Log::get().info("Signaling shutdown");
    Signals::HALT = true;
  }
}

void Miner::mine() {
  if (progress_monitor) {
    // start background thread for progress monitoring
    auto monitor = progress_monitor;
    std::thread monitor_thread([monitor]() {
      const auto delay = std::chrono::seconds(36);  // 1% steps (magic number)
      while (!monitor->isTargetReached() && !Signals::HALT) {
        monitor->writeProgress();
        std::this_thread::sleep_for(delay);
      }
      monitor->writeProgress();  // final write
      signalShutdown();
    });

    bool error = false;
    try {
      // load manager
      if (!manager) {
        reload();
      }
      // run main mining loop
      runMineLoop();
      auto mins = std::to_string(monitor->getElapsedSeconds() / 60);
      Log::get().info("Finished mining after " + mins + " minutes");
    } catch (const std::exception& e) {
      Log::get().error(
          "Error during initialization or mining: " + std::string(e.what()),
          false);
      signalShutdown();
      error = true;
    } catch (...) {
      Log::get().error("Unknown error during initialization or mining", false);
      signalShutdown();
      error = true;
    }
    try {
      monitor_thread.join();
    } catch (...) {
      Log::get().warn("Error joining progress monitoring thread");
    }
    if (error) {
      Log::get().error("Exiting due to error", true);  // exit with error
    }
  } else {
    // load manager
    if (!manager) {
      reload();
    }
    // run mining loop w/o monitoring
    runMineLoop();
    Log::get().info("Finished mining");
  }
}

std::string convertValidationModeToStr(ValidationMode mode) {
  switch (mode) {
    case ValidationMode::BASIC:
      return "basic";
    case ValidationMode::EXTENDED:
      return "extended";
  }
  return "";
}

void Miner::runMineLoop() {
  Parser parser;
  std::stack<Program> progs;
  std::stack<UID> maintain_ids;
  Sequence norm_seq;
  Submission submission;
  Program program;
  Matcher::seq_programs_t seq_programs;
  update_program_result_t update_result;

  // check validate modes
  if (validation_mode == ValidationMode::BASIC &&
      mining_mode == MiningMode::MINING_MODE_CLIENT) {
    Log::get().error("Basic validation not supported in client mining mode",
                     true);
  }

  // prepare base program
  std::string base_program_name;
  if (!base_program.ops.empty()) {
    if (mining_mode == MiningMode::MINING_MODE_SERVER) {
      Log::get().error("Mutate not supported in server mining mode", true);
    }
    if (!base_program.ops[0].comment.empty()) {
      base_program_name = base_program.ops[0].comment;
    }
    ProgramUtil::removeOps(base_program, Operation::Type::NOP);
    Comments::removeComments(base_program);

    // start with constants mutations; later do random mutation
    mutator->mutateCopiesConstants(base_program, NUM_MUTATIONS, progs);
  }

  // print info
  if (base_program.ops.empty()) {
    Log::get().info("Mining programs in " +
                    convertMiningModeToStr(mining_mode) + " mode, " +
                    convertValidationModeToStr(validation_mode) +
                    " validation mode");
  } else {
    std::string msg = "Mutating program";
    if (!base_program_name.empty()) {
      msg += " " + base_program_name;
    }
    Log::get().info(msg);
  }

  std::string submitter;
  std::string submitted_profile;
  current_fetch = (mining_mode == MINING_MODE_SERVER) ? PROGRAMS_TO_FETCH : 0;
  num_processed = 0;
  num_removed = 0;
  while (true) {
    // if queue is empty: fetch or generate a new program
    if (progs.empty()) {
      // server mode: try to fetch a program
      if (mining_mode == MINING_MODE_SERVER) {
        if (current_fetch > 0) {
          while (true) {
            submission = api_client->getNextSubmission();
            if (submission.mode == Submission::Mode::REMOVE) {
              maintain_ids.push(submission.id);
              continue;
            }
            program = submission.toProgram();
            if (program.ops.empty()) {
              current_fetch = 0;
              break;
            }
            current_fetch--;
            // check metadata stored in program's comments
            ensureSubmitter(program);
            progs.push(program);
            break;
          }
        }
      } else {
        // client mode
        if (base_program.ops.empty()) {
          // generate new program
          program = multi_generator->generateProgram();
          if (program.ops.empty() && multi_generator->isFinished()) {
            break;
          }
          progs.push(std::move(program));
        } else {
          // mutate base program
          mutator->mutateCopiesRandom(base_program, NUM_MUTATIONS, progs);
        }
      }
    }

    if (!progs.empty()) {
      // get the next program
      program = progs.top();
      progs.pop();

      // try to extract A-number from comment (server mode)
      seq_programs.clear();
      auto id_str = Comments::getSequenceIdFromProgram(program);
      if (!id_str.empty()) {
        UID id;
        bool ok = true;
        try {
          id = UID(id_str);
        } catch (const std::exception&) {
          ok = false;
        }
        if (id.domain() != 'A' || id.number() == 0) {
          ok = false;
        }
        if (ok) {
          seq_programs.push_back({id, program});
        } else {
          Log::get().warn("Invalid sequence ID: " + id_str);
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
        updateSubmitter(program);
        update_result =
            manager->updateProgram(s.first, program, validation_mode);
        if (update_result.updated) {
          // update metrics
          submitter = Comments::getSubmitter(program);
          if (submitter.empty()) {
            submitter = "unknown";
          }
          if (update_result.is_new) {
            num_new_per_user[submitter]++;
          } else {
            num_updated_per_user[submitter]++;
          }
          // in client mode: submit the program to the API server
          if (mining_mode == MINING_MODE_CLIENT) {
            if (s.first.domain() == 'A') {  // only A-numbers allowed
              // add metadata as comments
              program = update_result.program;
              Comments::addComment(
                  program, Comments::PREFIX_MINER_PROFILE + " " + profile_name);
              Comments::addComment(program, Comments::PREFIX_CHANGE_TYPE + " " +
                                                update_result.change_type);
              if (!update_result.is_new) {
                Comments::addComment(
                    program, Comments::PREFIX_PREVIOUS_HASH + " " +
                                 std::to_string(update_result.previous_hash));
              }
              api_client->postProgram(program, 10);  // magic number
            } else {
              Log::get().warn("Skipping program submission for " +
                              s.first.string());
            }
          }
          // mutate successful program
          if (mining_mode != MINING_MODE_SERVER && progs.size() < MAX_BACKLOG) {
            mutator->mutateCopiesConstants(update_result.program,
                                           NUM_MUTATIONS / 2, progs);
            mutator->mutateCopiesRandom(update_result.program,
                                        NUM_MUTATIONS / 2, progs);
          }
        }
      }
    } else {
      // we are in server mode and have no programs to process
      // => lets do maintenance work!
      if (maintain_ids.empty()) {
        maintain_ids.emplace(mutator->random_program_ids.getFromAll());
      }
      auto id = maintain_ids.top();
      maintain_ids.pop();
      if (!manager->maintainProgram(id)) {
        num_removed++;
      }
    }

    num_processed++;
    if (!checkRegularTasks()) {
      break;
    }
  }

  // final progress message
  logProgress(false);

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
    logProgress(true);
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

void Miner::logProgress(bool report_slow) {
  std::string progress;
  if (progress_monitor) {
    const double p = 100.0 * progress_monitor->getProgress();
    std::stringstream buf;
    buf.precision(1);
    buf << ", " << std::fixed << p << "%";
    progress = buf.str();
  }
  if (num_processed) {
    Log::get().info("Processed " + std::to_string(num_processed) + " programs" +
                    progress);
    num_processed = 0;
  } else if (report_slow) {
    Log::get().warn("Slow processing of programs" + progress);
  }
}

void Miner::reportCPUHour() {
  if (Setup::shouldReportCPUHours() && settings.report_cpu_hours) {
    api_client->postCPUHour();
  }
  num_reported_hours++;
}

void Miner::submit(const std::string& path, std::string id_str) {
  reload();
  Parser parser;
  auto program = parser.parse(path);
  submit_mode = true;
  if (id_str.empty()) {
    id_str = Comments::getSequenceIdFromProgram(program);
  }
  if (id_str.empty()) {
    Log::get().error("Missing sequence ID", true);
  }
  UID uid(id_str);
  if (uid.domain() != 'A') {  // only A-numbers allowed
    Log::get().error("Invalid sequence domain: " + id_str, true);
  }
  id_str = uid.string();
  Log::get().info("Loaded program for " + id_str);
  if (manager->isIgnored(uid)) {
    Log::get().error(
        "Sequence " + id_str + " is ignored by the active miner profile", true);
  }
  auto seq = ManagedSequence(uid);
  Settings settings(this->settings);
  settings.print_as_b_file = false;
  Evaluator evaluator(settings, EVAL_ALL, true);
  auto terms = seq.getTerms(SequenceUtil::FULL_SEQ_LENGTH);
  auto num_required = SequenceProgram::getNumRequiredTerms(program);
  Log::get().info(
      "Validating program against " + std::to_string(terms.size()) + " (>=" +
      std::to_string(std::min(num_required, terms.size())) + ") terms");
  auto result = evaluator.check(program, terms, num_required, uid);
  if (result.first == status_t::ERROR) {
    Log::get().error("Validation failed", false);
    settings.print_as_b_file = true;
    Evaluator evaluator2(settings, EVAL_ALL, true);
    evaluator2.check(program, terms, num_required, uid);
    return;  // error
  }
  Log::get().info("Validation successful");
  // match sequences
  Sequence norm_seq;
  const auto mode = Setup::getMiningMode();
  auto seq_programs = manager->getFinder().findSequence(
      program, norm_seq, manager->getSequences());
  Log::get().info("Found " + std::to_string(seq_programs.size()) +
                  " potential matches");
  size_t num_updated = 0;
  std::unordered_set<UID> updated_ids;
  for (auto s : seq_programs) {
    const std::string skip_msg = "Skipping submission for " + s.first.string();
    if (updated_ids.find(s.first) != updated_ids.end()) {
      Log::get().info(skip_msg + ": already updated");
      continue;
    }
    if (s.first.domain() != 'A') {  // only A-numbers allowed
      Log::get().info(skip_msg + ": invalid domain");
      continue;
    }
    program = s.second;
    updateSubmitter(program);
    auto r = manager->updateProgram(s.first, program, ValidationMode::EXTENDED);
    if (r.updated) {
      // in client mode: submit the program to the API server
      if (mode == MINING_MODE_CLIENT) {
        // add metadata as comment
        program = r.program;
        Comments::addComment(program,
                             Comments::PREFIX_MINER_PROFILE + " manual");
              Comments::addComment(program, Comments::PREFIX_CHANGE_TYPE + " " +
                                                r.change_type);
              if (!r.is_new) {
                Comments::addComment(
                    program, Comments::PREFIX_PREVIOUS_HASH + " " +
                                 std::to_string(r.previous_hash));
              }
        api_client->postProgram(program);
      } else {
        Log::get().info(skip_msg + ": not in client mode");
      }
      updated_ids.insert(s.first);
      num_updated++;
    } else {
      auto num_usages = manager->getStats().getNumUsages(uid);
      bool full_check = manager->isFullCheck(uid);
      auto existing = manager->getExistingProgram(s.first);
      auto msg = manager->getFinder().getChecker().compare(
          program, existing, "new", "existing", seq, full_check, num_usages);
      lowerString(msg);
      Log::get().info(skip_msg + ": " + msg);
    }
  }
  if (num_updated > 0) {
    if (mode == MINING_MODE_LOCAL) {
      Log::get().info("Stored " + std::to_string(num_updated) +
                      " programs in local programs directory");
      Log::get().warn("Skipping submissions to server due to local mode");
    } else {
      Log::get().info("Submitted " + std::to_string(num_updated) +
                      " programs to server");
    }
  } else {
    Log::get().info("No programs submitted to server");
  }
}

void Miner::ensureSubmitter(Program& program) {
  auto submitter = Comments::getSubmitter(program);
  if (submitter.empty()) {
    Comments::addComment(program,
                         Comments::PREFIX_SUBMITTED_BY + " " + UNKNOWN);
  }
}

void Miner::updateSubmitter(Program& program) {
  auto submitter = Comments::getSubmitter(program);
  if (submitter.empty()) {
    submitter = Setup::getSubmitter();
    if (!submitter.empty()) {
      Comments::addComment(program,
                           Comments::PREFIX_SUBMITTED_BY + " " + submitter);
    }
  } else if (submitter == UNKNOWN) {
    Comments::removeCommentField(program, Comments::PREFIX_SUBMITTED_BY);
  }
}
