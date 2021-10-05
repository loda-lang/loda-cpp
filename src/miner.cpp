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

Miner::Miner(const Settings &settings) : settings(settings) {}

void Miner::reload() {
  manager.reset(new OeisManager(settings));
  manager->load();
  manager->getFinder();  // initializes matchers
  multi_generator.reset(
      new MultiGenerator(settings, manager->getStats(), true));
}

void Miner::mine(const std::vector<std::string> &initial_progs) {
  if (!manager) {
    reload();
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
  static const std::string anonymous("anonymous");

  // load initial programs
  for (auto &path : initial_progs) {
    program = parser.parse(path);
    progs.push(program);
    mutator.mutateConstants(program, 1000, progs);
  }

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
  while (true) {
    // generate new program if needed
    if (progs.empty()) {
      multi_generator
          ->next();  // need to call "next" *before* generating the programs
      generator = multi_generator->getGenerator();
      progs.push(generator->generateProgram());
    }

    // get next program and match sequences
    program = progs.top();
    // Log::get().info( "Matching program with " + std::to_string(
    // program.ops.size() ) + " operations" ); ProgramUtil::print( program,
    // std::cout );
    progs.pop();
    auto seq_programs = manager->getFinder().findSequence(
        program, norm_seq, manager->getSequences());
    for (auto s : seq_programs) {
      program = s.second;

      // update "submitted by" comment
      submitted_by = ProgramUtil::getSubmittedBy(program);
      if (submitted_by.empty()) {
        submitted_by = Setup::getSubmittedBy();
        if (!submitted_by.empty()) {
          Operation nop(Operation::Type::NOP);
          nop.comment = ProgramUtil::SUBMITTED_BY_PREFIX + " " + submitted_by;
          program.ops.push_back(nop);
        }
      } else if (submitted_by == anonymous) {
        ProgramUtil::removeOps(program, Operation::Type::NOP);
      }

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
      for (size_t i = 0; i < 50; i++) {  // magic number
        program = api_client.getNextProgram();
        if (program.ops.empty()) {
          break;
        }
        // update "submitted by" comment
        submitted_by = ProgramUtil::getSubmittedBy(program);
        if (submitted_by.empty()) {
          Operation nop(Operation::Type::NOP);
          nop.comment = ProgramUtil::SUBMITTED_BY_PREFIX + " " + anonymous;
          program.ops.push_back(nop);
        }
        progs.push(program);
      }
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
      reload();
      generator = multi_generator->getGenerator();
    }
  }
}
