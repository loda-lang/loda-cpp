#pragma once

#include <memory>

#include "lang/number.hpp"
#include "lang/program.hpp"
#include "mine/stats.hpp"
#include "sys/util.hpp"

class Generator {
 public:
  using UPtr = std::unique_ptr<Generator>;

  class Config {
   public:
    int64_t version = 1;
    int64_t length = 0;
    int64_t max_constant = 0;
    int64_t max_index = 0;
    double mutation_rate = 0.0;
    bool loops = true;
    bool calls = true;
    bool indirect_access = false;
    std::vector<std::string> templates;
    std::string batch_file;
    std::string miner;
  };

  class Factory {
   public:
    static Generator::UPtr createGenerator(const Config &config,
                                           const Stats &stats);
  };

  Generator(const Config &config, const Stats &stats);

  virtual ~Generator() {}

  virtual Program generateProgram() = 0;

  virtual std::pair<Operation, double> generateOperation() = 0;

  virtual bool supportsRestart() const = 0;

  virtual bool isFinished() const = 0;

  const Config config;

  const RandomProgramIds2 random_program_ids;

 protected:
  void generateStateless(Program &p, size_t num_operations);

  void applyPostprocessing(Program &p);

  std::vector<int64_t> fixCausality(Program &p);

  void fixSingularities(Program &p);

  void fixCalls(Program &p);

  void ensureSourceNotOverwritten(Program &p);

  void ensureTargetWritten(Program &p,
                           const std::vector<int64_t> &written_cells);

  void ensureMeaningfulLoops(Program &p);
};

class MultiGenerator : public Generator {
 public:
  MultiGenerator(const Settings &settings, const Stats &stats, bool print_info);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

  virtual bool supportsRestart() const override;

  virtual bool isFinished() const override;

 private:
  std::vector<Generator::Config> configs;
  std::vector<Generator::UPtr> generators;
  size_t current_generator;
};
