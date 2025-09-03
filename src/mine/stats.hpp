#pragma once

#include <map>
#include <set>
#include <unordered_set>

#include "base/uid.hpp"
#include "eval/evaluator.hpp"
#include "mine/blocks.hpp"

class OpPos {
 public:
  Operation op;
  size_t pos;
  size_t len;

  inline bool operator==(const OpPos &o) const {
    return op == o.op && pos == o.pos && len == o.len;
  }

  inline bool operator!=(const OpPos &o) const { return !((*this) == o); }

  inline bool operator<(const OpPos &o) const {
    if (pos != o.pos) return pos < o.pos;
    if (len != o.len) return len < o.len;
    if (op != o.op) return op < o.op;
    return false;  // equal
  }
};

class Stats {
 public:
  static const std::string CALL_GRAPH_HEADER;
  static const std::string PROGRAMS_HEADER;
  static const std::string STEPS_HEADER;
  static const std::string SUMMARY_HEADER;

  Stats();

  void load(std::string path);

  void save(std::string path);

  std::string getMainStatsFile(std::string path) const;

  void updateProgramStats(UID id, const Program &program,
                          std::string submitter);

  void updateSequenceStats(UID id, bool program_found, bool formula_found);

  void finalize();

  int64_t getTransitiveLength(UID id) const;

  size_t getNumUsages(UID id) const;

  int64_t num_programs;
  int64_t num_sequences;
  int64_t num_formulas;
  steps_t steps;
  std::map<Number, int64_t> num_constants;
  std::map<Operation, int64_t> num_operations;
  std::map<OpPos, int64_t> num_operation_positions;
  std::map<std::string, int64_t> submitter_ref_ids;
  std::multimap<UID, UID> call_graph;
  std::vector<int64_t> num_programs_per_submitter;
  std::vector<int64_t> num_programs_per_length;
  std::vector<int64_t> num_ops_per_type;
  std::unordered_map<UID, int64_t> program_lengths;
  std::unordered_map<UID, int64_t> program_usages;
  std::unordered_map<UID, int64_t> program_submitter;
  UIDSet all_program_ids;
  UIDSet latest_program_ids;
  UIDSet supports_inceval;
  UIDSet supports_logeval;
  Blocks blocks;

 private:
  mutable std::set<UID> visited_programs;  // used for getTransitiveLength()
  mutable std::set<UID>
      printed_recursion_warning;  // used for getTransitiveLength()
  Blocks::Collector blocks_collector;
};

class RandomProgramIds {
 public:
  explicit RandomProgramIds(const UIDSet &ids);

  bool empty() const;
  bool exists(UID id) const;
  UID get() const;

 private:
  UIDSet ids_set;
  std::vector<UID> ids_vector;
};

class RandomProgramIds2 {
 public:
  explicit RandomProgramIds2(const Stats &stats);

  bool exists(UID id) const;
  UID get() const;
  UID getFromAll() const;

 private:
  RandomProgramIds all_program_ids;
  RandomProgramIds latest_program_ids;
};
