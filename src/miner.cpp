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

Miner::Miner(const Settings &settings)
    : settings(settings), manager(settings), interpreter(settings) {}

bool Miner::updateSpecialSequences(const Program &p,
                                   const Sequence &seq) const {
  std::string kind;
  if (isCollatzValuation(seq)) {
    // check again for longer sequence to avoid false alerts
    Evaluator evaluator(settings);
    Sequence seq2;
    evaluator.eval(p, seq2, OeisSequence::DEFAULT_SEQ_LENGTH, false);
    if (seq2.size() == OeisSequence::DEFAULT_SEQ_LENGTH &&
        isCollatzValuation(seq2)) {
      kind = "collatz";
    }
  }
  if (!kind.empty()) {
    Log::get().alert("Found possible " + kind +
                     " sequence: " + seq.to_string());
    std::string file_name = "programs/special/" + kind + "_" +
                            std::to_string(ProgramUtil::hash(p) % 1000000) +
                            ".asm";
    ensureDir(file_name);
    std::ofstream out(file_name);
    out << "; " << seq << std::endl;
    out << std::endl;
    ProgramUtil::print(p, out);
    out.close();
    return true;
  }
  return false;
}

bool Miner::isCollatzValuation(const Sequence &seq) {
  if (seq.size() < 10) {
    return false;
  }
  for (size_t i = 1; i < seq.size() - 1; i++) {
    size_t n = i + 1;
    if (n % 2 == 0)  // even
    {
      size_t j = (n / 2) - 1;  // >=0
      if (!(seq[j] < seq[i])) {
        return false;
      }
    } else  // odd
    {
      size_t j = (((3 * n) + 1) / 2) - 1;  // >=0
      if (j < seq.size() && !(seq[j] < seq[i])) {
        return false;
      }
    }
  }
  return true;
}

void Miner::mine() {
  manager.load();

  MultiGenerator multi_generator(settings, manager.getStats(), true);
  Mutator mutator;
  std::stack<Program> progs;
  Sequence norm_seq;
  auto &finder = manager.getFinder();
  AdaptiveScheduler metrics_scheduler(Metrics::get().publish_interval);
  AdaptiveScheduler api_scheduler(60);  // 1 minute (magic number)
  ApiClient api_client;

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
  Generator *generator = multi_generator.getGenerator();
  while (true) {
    // generate new program if needed
    if (progs.empty()) {
      multi_generator
          .next();  // need to call "next" *before* generating the programs
      generator = multi_generator.getGenerator();
      progs.push(generator->generateProgram());
    }

    // get next program and match sequences
    Program program = progs.top();
    // Log::get().info( "Matching program with " + std::to_string(
    // program.ops.size() ) + " operations" ); ProgramUtil::print( program,
    // std::cout );
    progs.pop();
    auto seq_programs =
        finder.findSequence(program, norm_seq, manager.getSequences());
    for (auto s : seq_programs) {
      auto r = manager.updateProgram(s.first, s.second);
      if (r.first) {
        // update stats and increase priority of successful generator
        if (r.second) {
          generator->stats.fresh++;
        } else {
          generator->stats.updated++;
        }
        // mutate successful program
        if (progs.size() < 1000 || Setup::hasMemory()) {
          mutator.mutateConstants(s.second, 100, progs);
        }
      }
    }
    if (updateSpecialSequences(program, norm_seq)) {
      generator->stats.fresh++;
    }
    generator->stats.generated++;

    // regular task: fetch programs from API server
    if (mode == MINING_MODE_SERVER && api_scheduler.isTargetReached()) {
      api_scheduler.reset();
      for (size_t i = 0; i < 10; i++) {  // magic number
        Program p = api_client.getNextProgram();
        if (p.ops.empty()) {
          break;
        }
        progs.push(p);
      }
    }

    // regular task: log info and publish metrics
    if (metrics_scheduler.isTargetReached()) {
      metrics_scheduler.reset();
      int64_t total_generated = 0;
      std::vector<Metrics::Entry> entries;
      for (size_t i = 0; i < multi_generator.generators.size(); i++) {
        auto gen = multi_generator.generators[i].get();
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
      finder.publishMetrics(entries);
      Metrics::get().write(entries);
    }
  }
}
