#include "mine/stats.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include "eval/evaluator_inc.hpp"
#include "lang/analyzer.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "oeis/oeis_program.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

const std::string Stats::CALL_GRAPH_HEADER("caller,callee");
const std::string Stats::PROGRAMS_HEADER("id,length,usages,inc_eval,log_eval");
const std::string Stats::STEPS_HEADER("total,min,max,runs");
const std::string Stats::SUMMARY_HEADER(
    "num_sequences,num_programs,num_formulas");

void checkHeader(std::istream &in, const std::string &header,
                 const std::string &file) {
  std::string line;
  if (!std::getline(in, line) || line != header) {
    throw std::runtime_error("unexpected header in " + file);
  }
}

Stats::Stats()
    : num_programs(0),
      num_sequences(0),
      num_formulas(0),
      num_ops_per_type(Operation::Types.size(), 0) {}

void Stats::load(std::string path) {
  ensureTrailingFileSep(path);
  Log::get().debug("Loading program stats from " + path);

  const std::string sep(",");
  std::string full, line, k, l, m, v, w;
  Parser parser;
  Operation op;
  Operand count;

  {
    full = path + "constant_counts.csv";
    Log::get().debug("Loading " + full);
    std::ifstream constants(full);
    while (std::getline(constants, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, v);
      num_constants[Number(k)] = std::stoll(v);
    }
    constants.close();
  }

  {
    full = path + "program_lengths.csv";
    Log::get().debug("Loading " + full);
    std::ifstream program_lengths(full);
    while (std::getline(program_lengths, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, v);
      auto l = std::stoll(k);
      while (l >= (int64_t)num_programs_per_length.size()) {
        num_programs_per_length.push_back(0);
      }
      num_programs_per_length[l] = std::stoll(v);
    }
    program_lengths.close();
  }

  {
    full = path + "operation_type_counts.csv";
    Log::get().debug("Loading " + full);
    std::ifstream op_type_counts(full);
    while (std::getline(op_type_counts, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, v);
      auto type = Operation::Metadata::get(k).type;
      num_ops_per_type.at(static_cast<size_t>(type)) = std::stoll(v);
    }
    op_type_counts.close();
  }

  {
    full = path + "operation_counts.csv";
    Log::get().debug("Loading " + full);
    std::ifstream op_counts(full);
    parser.in = &op_counts;
    while (true) {
      op_counts >> std::ws;
      if (op_counts.peek() == EOF) {
        break;
      }
      op.type = parser.readOperationType();
      parser.readSeparator(',');
      op.target = parser.readOperand();
      parser.readSeparator(',');
      op.source = parser.readOperand();
      parser.readSeparator(',');
      count = parser.readOperand();
      num_operations[op] = count.value.asInt();
    }
    op_counts.close();
  }

  {
    full = path + "operation_pos_counts.csv";
    Log::get().debug("Loading " + full);
    std::ifstream op_pos_counts(full);
    parser.in = &op_pos_counts;
    OpPos opPos;
    Operand pos, length;
    while (true) {
      op_pos_counts >> std::ws;
      if (op_pos_counts.peek() == EOF) {
        break;
      }
      pos = parser.readOperand();
      opPos.pos = pos.value.asInt();
      parser.readSeparator(',');
      length = parser.readOperand();
      opPos.len = length.value.asInt();
      parser.readSeparator(',');
      opPos.op.type = parser.readOperationType();
      parser.readSeparator(',');
      opPos.op.target = parser.readOperand();
      parser.readSeparator(',');
      opPos.op.source = parser.readOperand();
      parser.readSeparator(',');
      count = parser.readOperand();
      num_operation_positions[opPos] = count.value.asInt();
    }
    op_pos_counts.close();
  }

  {
    full = path + "programs.csv";
    Log::get().debug("Loading " + full);
    std::ifstream programs(full);
    resizeProgramLists(100000);
    int64_t largest_id = 0;
    checkHeader(programs, PROGRAMS_HEADER, full);
    while (std::getline(programs, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, l, ',');
      std::getline(s, m, ',');
      std::getline(s, v, ',');
      std::getline(s, w);
      int64_t id = std::stoll(k);
      largest_id = std::max<int64_t>(largest_id, id);
      resizeProgramLists(id);
      all_program_ids[id] = true;
      program_lengths[id] = std::stoll(l);
      program_usages[id] = std::stoll(m);
      supports_inceval[id] = std::stoll(v);
      supports_logeval[id] = std::stoll(w);
    }
    size_t new_size = largest_id + 1;
    all_program_ids.resize(new_size);
    program_lengths.resize(new_size);
    supports_inceval.resize(new_size);
    supports_logeval.resize(new_size);
    programs.close();
  }

  {
    full = path + "latest_programs.csv";
    Log::get().debug("Loading " + full);
    std::ifstream latest_programs(full);
    latest_program_ids.clear();
    while (std::getline(latest_programs, line)) {
      int64_t id = std::stoll(line);
      if (id < 0 || id > 1000000) {
        throw std::runtime_error("unexpected latest program ID: " +
                                 std::to_string(id));
      }
      if (id >= static_cast<int64_t>(latest_program_ids.size())) {
        const size_t new_size =
            std::max<size_t>(id + 1, 2 * latest_program_ids.size());
        latest_program_ids.resize(new_size);
      }
      latest_program_ids[id] = true;
    }
    latest_programs.close();
  }

  {
    full = path + "call_graph.csv";
    Log::get().debug("Loading " + full);
    std::ifstream call(full);
    checkHeader(call, CALL_GRAPH_HEADER, full);
    call_graph.clear();
    while (std::getline(call, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, v);
      call_graph.insert(
          std::pair<int64_t, int64_t>(OeisSequence(k).id, OeisSequence(v).id));
    }
    call.close();
  }

  {
    full = path + "summary.csv";
    Log::get().debug("Loading " + full);
    std::ifstream summary(full);
    checkHeader(summary, SUMMARY_HEADER, full);
    std::getline(summary, k, ',');
    std::getline(summary, v, ',');
    std::getline(summary, w);
    num_sequences = std::stoll(k);
    num_programs = std::stoll(v);
    num_formulas = std::stoll(w);
  }

  { blocks.load(path + "blocks.asm"); }

  // TODO: remaining stats

  Log::get().debug("Finished loading program stats");
}

void Stats::save(std::string path) {
  ensureTrailingFileSep(path);
  Log::get().debug("Saving program stats to " + path);

  const std::string sep(",");
  std::ofstream constants(path + "constant_counts.csv");
  for (auto &e : num_constants) {
    constants << e.first << sep << e.second << "\n";
  }
  constants.close();

  std::ofstream programs(path + "programs.csv");
  programs << PROGRAMS_HEADER << "\n";
  for (size_t id = 0; id < all_program_ids.size(); id++) {
    if (all_program_ids[id]) {
      programs << id << sep << program_lengths[id] << sep << program_usages[id]
               << sep << supports_inceval[id] << sep << supports_logeval[id]
               << "\n";
    }
  }
  programs.close();

  std::ofstream latest_programs(path + "latest_programs.csv");
  for (size_t id = 0; id < latest_program_ids.size(); id++) {
    if (latest_program_ids[id]) {
      latest_programs << id << "\n";
    }
  }
  latest_programs.close();

  std::ofstream lengths(path + "program_lengths.csv");
  for (size_t i = 0; i < num_programs_per_length.size(); i++) {
    if (num_programs_per_length[i] > 0) {
      lengths << i << sep << num_programs_per_length[i] << "\n";
    }
  }
  lengths.close();

  std::ofstream op_type_counts(path + "operation_type_counts.csv");
  for (size_t i = 0; i < num_ops_per_type.size(); i++) {
    if (num_ops_per_type[i] > 0) {
      op_type_counts
          << Operation::Metadata::get(static_cast<Operation::Type>(i)).name
          << sep << num_ops_per_type[i] << "\n";
    }
  }
  op_type_counts.close();

  std::ofstream op_counts(path + "operation_counts.csv");
  for (auto &op : num_operations) {
    const auto &meta = Operation::Metadata::get(op.first.type);
    op_counts << meta.name << sep
              << ProgramUtil::operandToString(op.first.target) << sep
              << ProgramUtil::operandToString(op.first.source) << sep
              << op.second << "\n";
  }
  op_counts.close();

  std::ofstream oppos_counts(path + "operation_pos_counts.csv");
  for (auto &o : num_operation_positions) {
    const auto &meta = Operation::Metadata::get(o.first.op.type);
    oppos_counts << o.first.pos << sep << o.first.len << sep << meta.name << sep
                 << ProgramUtil::operandToString(o.first.op.target) << sep
                 << ProgramUtil::operandToString(o.first.op.source) << sep
                 << o.second << "\n";
  }
  oppos_counts.close();

  std::ofstream summary(path + "summary.csv");
  summary << SUMMARY_HEADER << "\n";
  summary << num_sequences << sep << num_programs << sep << num_formulas
          << "\n";
  summary.close();

  std::ofstream cal(path + "call_graph.csv");
  cal << CALL_GRAPH_HEADER << "\n";
  for (auto it : call_graph) {
    cal << ProgramUtil::idStr(it.first) << sep << ProgramUtil::idStr(it.second)
        << "\n";
  }
  cal.close();

  if (steps.total) {  // write steps stats only if present
    std::ofstream steps_out(path + "steps.csv");
    steps_out << STEPS_HEADER << "\n";
    steps_out << steps.total << sep << steps.min << sep << steps.max << sep
              << steps.runs << "\n";
    steps_out.close();
  }

  { blocks.save(path + "blocks.asm"); }

  Log::get().debug("Finished saving program stats");
}

std::string Stats::getMainStatsFile(std::string path) const {
  ensureTrailingFileSep(path);
  path += "constant_counts.csv";
  return path;
}

void Stats::updateProgramStats(size_t id, const Program &program) {
  const size_t num_ops = ProgramUtil::numOps(program, false);
  resizeProgramLists(id);
  program_lengths[id] = num_ops;
  if (num_ops >= num_programs_per_length.size()) {
    num_programs_per_length.resize(num_ops + 1);
  }
  num_programs_per_length[num_ops]++;
  OpPos o;
  o.len = program.ops.size();
  o.pos = 0;
  for (auto &op : program.ops) {
    num_ops_per_type[static_cast<size_t>(op.type)]++;
    if (op.type != Operation::Type::SEQ &&
        Operation::Metadata::get(op.type).num_operands == 2 &&
        op.source.type == Operand::Type::CONSTANT) {
      if (num_constants.find(op.source.value) == num_constants.end()) {
        num_constants[op.source.value] = 0;
      }
      num_constants[op.source.value]++;
    }
    if (op.type != Operation::Type::NOP) {
      if (num_operations.find(op) == num_operations.end()) {
        num_operations[op] = 1;
      } else {
        num_operations[op]++;
      }
      o.op = op;
      if (num_operation_positions.find(o) == num_operation_positions.end()) {
        num_operation_positions[o] = 1;
      } else {
        num_operation_positions[o]++;
      }
    }
    if (op.type == Operation::Type::SEQ &&
        op.source.type == Operand::Type::CONSTANT) {
      auto called = op.source.value.asInt();
      resizeProgramLists(called);
      call_graph.insert(std::pair<int64_t, int64_t>(id, called));
      program_usages[called]++;
    }
    o.pos++;
  }
  Settings settings;
  Interpreter interpreter(settings);
  IncrementalEvaluator inceval(interpreter);
  supports_inceval[id] = inceval.init(program);
  supports_logeval[id] = Analyzer::hasLogarithmicComplexity(program);
  blocks_collector.add(program);
}

void Stats::updateSequenceStats(size_t id, bool program_found,
                                bool formula_found) {
  num_sequences++;
  num_programs += static_cast<int64_t>(program_found);
  num_formulas += static_cast<int64_t>(formula_found);
  resizeProgramLists(id);
  all_program_ids[id] = program_found;
}

void Stats::resizeProgramLists(size_t id) {
  if (id >= all_program_ids.size()) {
    const size_t new_size =
        std::max<size_t>(id + 1, 2 * all_program_ids.size());
    all_program_ids.resize(new_size);
    program_lengths.resize(new_size);
    program_usages.resize(new_size);
    supports_inceval.resize(new_size);
    supports_logeval.resize(new_size);
  }
}

void Stats::finalize() {
  if (!blocks_collector.empty()) {
    if (!blocks.list.ops.empty()) {
      Log::get().error("Attempted overwrite of blocks stats", true);
    }
    blocks = blocks_collector.finalize();
  }
  if (latest_program_ids.empty()) {
    latest_program_ids =
        OeisProgram::collectLatestProgramIds(20, 200, 200);  // magic number
  }
}

int64_t Stats::getTransitiveLength(size_t id) const {
  if (visited_programs.find(id) != visited_programs.end()) {
    visited_programs.clear();
    if (printed_recursion_warning.find(id) == printed_recursion_warning.end()) {
      printed_recursion_warning.insert(id);
      Log::get().warn("Recursion detected: " + ProgramUtil::idStr(id));
    }
    return -1;
  }
  visited_programs.insert(id);
  if (id >= program_lengths.size()) {
    Log::get().warn("Invalid reference: " + ProgramUtil::idStr(id));
    return -1;
  }
  int64_t length = program_lengths.at(id);
  auto range = call_graph.equal_range(id);
  for (auto &it = range.first; it != range.second; it++) {
    length += getTransitiveLength(it->second);
  }
  visited_programs.erase(id);
  return length;
}

RandomProgramIds::RandomProgramIds(const std::vector<bool> &flags) {
  for (size_t id = 0; id < flags.size(); id++) {
    if (flags[id]) {
      ids_vector.push_back(id);
      ids_set.insert(id);
    }
  }
}

bool RandomProgramIds::empty() const { return ids_set.empty(); }

bool RandomProgramIds::exists(int64_t id) const {
  return (id >= 0) && (ids_set.find(id) != ids_set.end());
}

int64_t RandomProgramIds::get() const {
  if (!ids_vector.empty()) {
    return ids_vector[Random::get().gen() % ids_vector.size()];
  }
  return 0;
}

RandomProgramIds2::RandomProgramIds2(const Stats &stats)
    : all_program_ids(stats.all_program_ids),
      latest_program_ids(stats.latest_program_ids) {}

bool RandomProgramIds2::exists(int64_t id) const {
  return all_program_ids.exists(id) || latest_program_ids.exists(id);
}

int64_t RandomProgramIds2::get() const {
  if (Random::get().gen() % 2 == 0 ||
      latest_program_ids.empty()) {  // magic number
    return all_program_ids.get();
  } else {
    return latest_program_ids.get();
  }
}

int64_t RandomProgramIds2::getFromAll() const { return all_program_ids.get(); }
