#pragma once

#include "evaluator.hpp"
#include "finder.hpp"
#include "minimizer.hpp"
#include "number.hpp"
#include "oeis_sequence.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "stats.hpp"
#include "util.hpp"

enum class OverwriteMode { NONE, ALL, AUTO };
enum class ValidationMode { BASIC, EXTENDED };

struct update_program_result_t {
  bool updated;
  bool is_new;
  size_t previous_hash;
  std::string change_type;
  Program program;
};

class OeisManager {
 public:
  OeisManager(const Settings& settings, const std::string& stats_home = "");

  void load();

  void migrate();

  const std::vector<OeisSequence>& getSequences() const;

  const Stats& getStats();

  void releaseStats();

  void setStatsHome(const std::string& home);

  Finder& getFinder();

  size_t getTotalCount() const { return total_count; }

  update_program_result_t updateProgram(size_t id, Program p,
                                        ValidationMode validation_mode);

  bool maintainProgram(size_t id);

  void generateLists();

 private:
  void loadData();

  void loadNames();

  bool shouldMatch(const OeisSequence& seq) const;

  void update();

  void generateStats(int64_t age_in_days);

  void addSeqComments(Program& p) const;

  void dumpProgram(size_t id, Program& p, const std::string& file,
                   const std::string& submitted_by) const;

  void alert(Program p, size_t id, const std::string& prefix,
             const std::string& color, const std::string& submitted_by,
             bool tweet) const;

  friend class OeisMaintenance;

  const Settings& settings;
  const OverwriteMode overwrite_mode;
  Parser parser;
  Evaluator evaluator;
  Finder finder;
  bool finder_initialized;
  bool update_oeis;
  bool update_programs;

  Optimizer optimizer;
  Minimizer minimizer;
  std::vector<OeisSequence> sequences;

  std::unordered_set<size_t> deny_list;
  std::unordered_set<size_t> overwrite_list;
  std::unordered_set<size_t> protect_list;
  std::unordered_set<size_t> ignore_list;
  std::unordered_set<size_t> full_check_list;
  std::map<size_t, int64_t> invalid_matches_map;

  size_t loaded_count;
  size_t total_count;

  std::unique_ptr<Stats> stats;
  std::string stats_home;
};
