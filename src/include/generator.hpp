#pragma once

#include "number.hpp"
#include "program.hpp"
#include "stats.hpp"
#include "util.hpp"

#include <memory>

class Generator
{
public:

  using UPtr = std::unique_ptr<Generator>;

  class Config
  {
  public:
    int64_t version = 1;
    int64_t replicas = 1;
    int64_t length = 0;
    int64_t max_constant = 0;
    int64_t max_index = 0;
    bool loops = true;
    bool calls = true;
    bool indirect_access = false;
    std::string program_template;
  };

  class GStats
  {
  public:
    GStats()
        : generated( 0 ),
          fresh( 0 ),
          updated( 0 )
    {
    }

    int64_t generated;
    int64_t fresh;
    int64_t updated;
  };

  class Factory
  {
  public:
    static Generator::UPtr createGenerator( const Config &config, const Stats &stats, int64_t seed );
  };

  Generator( const Config &config, const Stats &stats, int64_t seed );

  virtual ~Generator()
  {
  }

  virtual Program generateProgram() = 0;

  virtual std::pair<Operation, double> generateOperation() = 0;

  const Config config;

  const std::vector<bool> found_programs;

  std::map<std::string, std::string> metric_labels;

  GStats stats;

protected:

  void generateStateless( Program &p, size_t num_operations );

  void applyPostprocessing( Program &p );

  std::vector<number_t> fixCausality( Program &p );

  void fixSingularities( Program &p );

  void fixCalls( Program &p );

  void ensureSourceNotOverwritten( Program &p );

  void ensureTargetWritten( Program &p, const std::vector<number_t> &written_cells );

  void ensureMeaningfulLoops( Program &p );

  std::mt19937 gen;

};

class MultiGenerator
{
public:

  MultiGenerator( const Settings &settings, const Stats& stats, int64_t seed );

  Generator* getGenerator();

  void next();

  std::vector<Generator::Config> configs;
  std::vector<Generator::UPtr> generators;
  int64_t generator_index;
  int64_t replica_index;

};
