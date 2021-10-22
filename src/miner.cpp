#include "miner.hpp"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include "api_client.hpp"
#include "file.hpp"
#include "generator.hpp"
#include "interpreter.hpp"
#include "mutator.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"

const std::string Miner::ANONYMOUS("anonymous");

Miner::Miner(const Settings &settings) : settings(settings) {}

void Miner::reload(bool load_generators, bool force_overwrite) {
  manager.reset(new OeisManager(settings, force_overwrite));
  manager->load();
  manager->getFinder();  // initializes matchers
  if (load_generators) {
    multi_generator.reset(
        new MultiGenerator(settings, manager->getStats(), true));
  } else {
    multi_generator.reset();
  }
}

void Miner::mine() {
  if (!manager) {
    reload(true);
  }
  ApiClient api_client;
  Mutator mutator;
  Parser parser;
  std::stack<Program> progs;
  Sequence norm_seq;
  Program program;
  AdaptiveScheduler metrics_scheduler(Metrics::get().publish_interval);
  AdaptiveScheduler api_scheduler(600);       // 10 minutes (magic number)
  AdaptiveScheduler reload_scheduler(43200);  // 12 hours (magic number)

  // get mining mode
  const auto mode = Setup::getMiningMode();
  std::string mode_str;
  switch (mode) {
    case MINING_MODE_LOCAL:
      mode_str = "local";
      break;
    case MINING_MODE_CLIENT:
      mode_str = "client";
      break;
    case MINING_MODE_SERVER:
      mode_str = "server";
      break;
  }

  Log::get().info("Mining programs for OEIS sequences in " + mode_str +
                  " mode");
  Generator *generator = multi_generator->getGenerator();
  std::string submitted_by;
  const int64_t programs_to_fetch = 50;  // magic number
  int64_t current_fetch = (mode == MINING_MODE_SERVER) ? programs_to_fetch : 0;
  while (true) {
    // server mode: fetch new program
    if (progs.empty() && current_fetch > 0 && mode == MINING_MODE_SERVER) {
      program = api_client.getNextProgram();
      if (program.ops.empty()) {
        current_fetch = 0;
      } else {
        current_fetch--;
        ensureSubmittedBy(program);
        progs.push(program);
      }
    }

    // generate new program if needed
    if (progs.empty()) {
      multi_generator
          ->next();  // need to call "next" *before* generating the programs
      generator = multi_generator->getGenerator();
      progs.push(generator->generateProgram());
    }

    // get next program
    program = progs.top();
    progs.pop();
    // Log::get().info( "Matching program with " + std::to_string(
    // program.ops.size() ) + " operations" ); ProgramUtil::print( program,
    // std::cout );

    // match sequences
    auto seq_programs = manager->getFinder().findSequence(
        program, norm_seq, manager->getSequences());
    for (auto s : seq_programs) {
      program = s.second;
      setSubmittedBy(program);
      auto r = manager->updateProgram(s.first, program);
      if (r.first) {
        // update stats and increase priority of successful generator
        if (r.second) {
          generator->stats.fresh++;
        } else {
          generator->stats.updated++;
        }
        // in client mode: submit the program to the API server
        if (mode == MINING_MODE_CLIENT) {
          api_client.postProgram(program);
        }
        // mutate successful program
        if (progs.size() < 1000 || Setup::hasMemory()) {
          mutator.mutateConstants(program, 100, progs);
        }
      }
    }
    generator->stats.generated++;

    // regular task: fetch programs from API server
    if (mode == MINING_MODE_SERVER && api_scheduler.isTargetReached()) {
      api_scheduler.reset();
      current_fetch += programs_to_fetch;
    }

    // regular task: log info and publish metrics
    if (metrics_scheduler.isTargetReached()) {
      metrics_scheduler.reset();
      int64_t total_generated = 0;
      std::vector<Metrics::Entry> entries;
      for (size_t i = 0; i < multi_generator->generators.size(); i++) {
        auto gen = multi_generator->generators[i].get();
        auto labels = gen->metric_labels;
        labels["kind"] = "generated";
        entries.push_back({"programs", labels, (double)gen->stats.generated});
        labels["kind"] = "new";
        entries.push_back({"programs", labels, (double)gen->stats.fresh});
        labels["kind"] = "updated";
        entries.push_back({"programs", labels, (double)gen->stats.updated});
        total_generated += gen->stats.generated;
        gen->stats = Generator::GStats();
      }
      Log::get().info("Generated " + std::to_string(total_generated) +
                      " programs");
      manager->getFinder().publishMetrics(entries);
      Metrics::get().write(entries);
    }

    // regular task: reload oeis manager and generators
    if (reload_scheduler.isTargetReached()) {
      reload_scheduler.reset();
      reload(true);
      generator = multi_generator->getGenerator();
    }
  }
}

void Miner::submit(const std::string &id, const std::string &path) {
  Parser parser;
  auto program = parser.parse(path);
  reload(false, true);
  OeisSequence seq(id);
  Settings settings(this->settings);
  settings.print_as_b_file = false;
  Log::get().info("Validating program for " + seq.id_str());
  Evaluator evaluator(settings);
  auto terms = seq.getTerms(100000);  // magic number
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
  ApiClient api_client;
  Sequence norm_seq;
  const auto mode = Setup::getMiningMode();
  auto seq_programs = manager->getFinder().findSequence(
      program, norm_seq, manager->getSequences());
  size_t matches = 0;
  for (auto s : seq_programs) {
    program = s.second;
    setSubmittedBy(program);
    auto r = manager->updateProgram(s.first, program);
    if (r.first) {
      // in client mode: submit the program to the API server
      if (mode == MINING_MODE_CLIENT) {
        api_client.postProgram(program);
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
  auto submitted_by = ProgramUtil::getSubmittedBy(program);
  if (submitted_by.empty()) {
    Operation nop(Operation::Type::NOP);
    nop.comment = ProgramUtil::SUBMITTED_BY_PREFIX + " " + ANONYMOUS;
    program.ops.push_back(nop);
  }
}

void Miner::setSubmittedBy(Program &program) {
  auto submitted_by = ProgramUtil::getSubmittedBy(program);
  if (submitted_by.empty()) {
    submitted_by = Setup::getSubmittedBy();
    if (!submitted_by.empty()) {
      Operation nop(Operation::Type::NOP);
      nop.comment = ProgramUtil::SUBMITTED_BY_PREFIX + " " + submitted_by;
      program.ops.push_back(nop);
    }
  } else if (submitted_by == ANONYMOUS) {
    ProgramUtil::removeOps(program, Operation::Type::NOP);
  }
}
