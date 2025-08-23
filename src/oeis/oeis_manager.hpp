#pragma once

#include "eval/evaluator.hpp"
#include "eval/minimizer.hpp"
#include "eval/optimizer.hpp"
#include "lang/parser.hpp"
#include "lang/program_cache.hpp"
#include "mine/finder.hpp"
#include "mine/stats.hpp"
#include "oeis/invalid_matches.hpp"
#include "seq/managed_sequence.hpp"
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

  const SequenceIndex& getSequences() const;

  const Stats& getStats();

  Finder& getFinder();

  size_t getTotalCount() const { return total_count; }

  Program getExistingProgram(UID id);

  update_program_result_t updateProgram(UID id, Program p,
                                        ValidationMode validation_mode);

  bool maintainProgram(UID id, bool eval = true);

  void dumpProgram(UID id, Program& p, const std::string& file,
                   const std::string& submitted_by) const;

  void generateLists();

  std::vector<Program> loadAllPrograms();

  bool isIgnored(UID id) const {
    return ignore_list.find(id) != ignore_list.end();
  }

  bool isFullCheck(UID id) const {
    return full_check_list.find(id) != full_check_list.end();
  }

 private:
  void loadData();

  void loadNames();

  void checkConsistency() const;

  void loadOffsets();

  bool shouldMatch(const ManagedSequence& seq) const;

  void generateStats(int64_t age_in_days);

  void addSeqComments(Program& p) const;

  int64_t updateProgramOffset(UID id, Program& p) const;

  void updateDependentOffset(UID id, UID used_id, int64_t delta);

  void updateAllDependentOffset(UID id, int64_t delta);

  void alert(Program p, UID id, const std::string& prefix,
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
  SequenceIndex sequences;

  std::unordered_set<UID> deny_list;
  std::unordered_set<UID> overwrite_list;
  std::unordered_set<UID> protect_list;
  std::unordered_set<UID> ignore_list;
  std::unordered_set<UID> full_check_list;
  InvalidMatches invalid_matches;

  size_t loaded_count;
  size_t total_count;

  std::unique_ptr<Stats> stats;
  std::string stats_home;
};
