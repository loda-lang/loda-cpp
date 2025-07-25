#pragma once

#include "eval/evaluator.hpp"
#include "eval/minimizer.hpp"
#include "eval/optimizer.hpp"
#include "lang/parser.hpp"
#include "lang/program_cache.hpp"
#include "mine/finder.hpp"
#include "mine/stats.hpp"
#include "oeis/invalid_matches.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/util.hpp"

enum class OverwriteMode { NONE, ALL, AUTO };
enum class ValidationMode { BASIC, EXTENDED };

std::string OverrideModeToString(OverwriteMode mode);

struct update_program_result_t {
  bool updated;
  bool is_new;
  size_t previous_hash;
  std::string change_type;
  Program program;
};

class OeisManager {
 public:
  explicit OeisManager(const Settings& settings,
                       const std::string& stats_home = "");

  void load();

  void update(bool force);

  void migrate();

  const std::vector<OeisSequence>& getSequences() const;

  const Stats& getStats();

  Finder& getFinder();

  size_t getTotalCount() const { return total_count; }

  Program getExistingProgram(size_t id);

  update_program_result_t updateProgram(size_t id, Program p,
                                        ValidationMode validation_mode);

  bool maintainProgram(size_t id, bool eval = true);

  void dumpProgram(size_t id, Program& p, const std::string& file,
                   const std::string& submitted_by) const;

  void generateLists();

  std::vector<Program> loadAllPrograms();

  bool isIgnored(size_t id) const {
    return ignore_list.find(id) != ignore_list.end();
  }

  bool isFullCheck(size_t id) const {
    return full_check_list.find(id) != full_check_list.end();
  }

 private:
  void loadData();

  void loadNames();

  void loadOffsets();

  bool shouldMatch(const OeisSequence& seq) const;

  void generateStats(int64_t age_in_days);

  void addSeqComments(Program& p) const;

  int64_t updateProgramOffset(size_t id, Program& p) const;

  void updateDependentOffset(size_t id, size_t used_id, int64_t delta);

  void updateAllDependentOffset(size_t id, int64_t delta);

  void alert(Program p, size_t id, const std::string& prefix,
             const std::string& color, const std::string& submitted_by) const;

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
  InvalidMatches invalid_matches;

  size_t loaded_count;
  size_t total_count;

  std::unique_ptr<Stats> stats;
  std::string stats_home;
};
