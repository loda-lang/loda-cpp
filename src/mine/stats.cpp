#include "mine/stats.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "eval/evaluator_inc.hpp"
#include "lang/analyzer.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "seq/managed_seq.hpp"
#include "seq/seq_program.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

const std::string Stats::CALL_GRAPH_HEADER("caller,callee");
const std::string Stats::PROGRAMS_HEADER(
    "id,submitter,length,usages,inc_eval,log_eval,vir_eval,loop,formula");
const std::string Stats::STEPS_HEADER("total,min,max,runs");
const std::string Stats::SUMMARY_HEADER(
    "num_sequences,num_programs,num_formulas");
const std::string SUBMITTERS_HEADER = "submitter,ref_id,num_programs";

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
  auto start_time = std::chrono::steady_clock::now();

  const std::string sep(",");
  std::string full, line, k, l, m, v, w, u, x;
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
    int64_t largest_id = 0;
    checkHeader(programs, PROGRAMS_HEADER, full);
    std::string loop_col, formula_col;
    while (std::getline(programs, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, u, ',');
      std::getline(s, l, ',');
      std::getline(s, m, ',');
      std::getline(s, v, ',');
      std::getline(s, w, ',');
      std::getline(s, x, ',');
      std::getline(s, loop_col, ',');
      std::getline(s, formula_col);
      UID id(k);
      largest_id = std::max<int64_t>(largest_id, id.number());
      all_program_ids.insert(id);
      program_submitter[id] = std::stoll(u);
      program_lengths[id] = std::stoll(l);
      program_usages[id] = std::stoll(m);
      if (std::stoll(v)) {
        supports_inceval.insert(id);
      }
      if (std::stoll(w)) {
        supports_logeval.insert(id);
      }
      if (std::stoll(loop_col)) {
        has_loop.insert(id);
      }
      if (std::stoll(formula_col)) {
        has_formula.insert(id);
      }
    }
    programs.close();
  }

  {
    full = path + "latest_programs.csv";
    Log::get().debug("Loading " + full);
    std::ifstream latest_programs(full);
    latest_program_ids.clear();
    while (std::getline(latest_programs, line)) {
      latest_program_ids.insert(UID(line));
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
      call_graph.insert(std::pair<UID, UID>(UID(k), UID(v)));
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

  {
    blocks.load(path + "blocks.asm");
  }

  {
    full = path + "submitters.csv";
    Log::get().debug("Loading " + full);
    std::ifstream submitters(full);
    num_programs_per_submitter.clear();
    checkHeader(submitters, SUBMITTERS_HEADER, full);
    while (std::getline(submitters, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, v, ',');
      std::getline(s, w);
      auto ref_id = std::stoll(v);
      submitter_ref_ids[k] = ref_id;
      if (ref_id >= static_cast<int64_t>(num_programs_per_submitter.size())) {
        num_programs_per_submitter.resize(ref_id + 1);
      }
      num_programs_per_submitter[ref_id] = std::stoll(w);
    }
    submitters.close();
  }

  // TODO: remaining stats

  auto cur_time = std::chrono::steady_clock::now();
  double duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        cur_time - start_time)
                        .count() /
                    1000.0;
  std::stringstream buf;
  buf.setf(std::ios::fixed);
  buf.precision(2);
  buf << duration;
  Log::get().info("Loaded stats for " + std::to_string(num_programs) +
                  " programs in " + buf.str() + "s");
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
  for (auto id : all_program_ids) {
    const auto inceval = supports_inceval.exists(id);
    const auto logeval = supports_logeval.exists(id);
    const auto vireval = supports_vireval.exists(id);
    const auto loop_flag = has_loop.exists(id);
    const auto formula_flag = has_formula.exists(id);
    programs << id.string() << sep << program_submitter[id] << sep
             << program_lengths[id] << sep << program_usages[id] << sep
             << inceval << sep << logeval << sep << vireval << sep << loop_flag << sep
             << formula_flag << "\n";
  }
  programs.close();

  std::ofstream latest_programs(path + "latest_programs.csv");
  for (auto id : latest_program_ids) {
    latest_programs << id.string() << "\n";
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
    cal << it.first.string() << sep << it.second.string() << "\n";
  }
  cal.close();

  if (steps.total) {  // write steps stats only if present
    std::ofstream steps_out(path + "steps.csv");
    steps_out << STEPS_HEADER << "\n";
    steps_out << steps.total << sep << steps.min << sep << steps.max << sep
              << steps.runs << "\n";
    steps_out.close();
  }

  {
    blocks.save(path + "blocks.asm");
  }

  std::ofstream submitters(path + "submitters.csv");
  submitters << SUBMITTERS_HEADER << "\n";
  for (const auto &e : submitter_ref_ids) {
    submitters << e.first << sep << e.second << sep
               << num_programs_per_submitter[e.second] << "\n";
  }
  submitters.close();

  Log::get().debug("Finished saving program stats");
}

std::string Stats::getMainStatsFile(std::string path) const {
  ensureTrailingFileSep(path);
  path += "constant_counts.csv";
  return path;
}

void Stats::updateProgramStats(UID id, const Program &program,
                               std::string submitter, bool with_formula) {
  const size_t num_ops = ProgramUtil::numOps(program, false);
  program_lengths[id] = num_ops;
  if (num_ops >= num_programs_per_length.size()) {
    num_programs_per_length.resize(num_ops + 1);
  }
  num_programs_per_length[num_ops]++;
  int64_t ref_id;
  replaceAll(submitter, ",", "_");
  auto it = submitter_ref_ids.find(submitter);
  if (it != submitter_ref_ids.end()) {
    ref_id = it->second;
  } else {
    ref_id = submitter_ref_ids.size() + 1;
    submitter_ref_ids[submitter] = ref_id;
    if (ref_id >= static_cast<int64_t>(num_programs_per_submitter.size())) {
      num_programs_per_submitter.resize(ref_id + 1, 0);
    }
  }
  num_programs_per_submitter[ref_id]++;
  program_submitter[id] = ref_id;
  OpPos o;
  o.len = program.ops.size();
  o.pos = 0;
  bool with_loop = false;
  for (auto &op : program.ops) {
    num_ops_per_type[static_cast<size_t>(op.type)]++;
    if (op.type == Operation::Type::LPB) {
      with_loop = true;
    }
    if (op.type != Operation::Type::SEQ && op.type != Operation::Type::PRG &&
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
    if ((op.type == Operation::Type::SEQ || op.type == Operation::Type::PRG) &&
        op.source.type == Operand::Type::CONSTANT) {
      auto called = UID::castFromInt(op.source.value.asInt());
      call_graph.insert(std::pair<UID, UID>(id, called));
      program_usages[called]++;
    }
    o.pos++;
  }
  Settings settings;
  Interpreter interpreter(settings);
  IncrementalEvaluator inceval(interpreter);
  VirtualEvaluator vireval(settings);
  if (inceval.init(program)) {
    supports_inceval.insert(id);
  }
  if (Analyzer::hasLogarithmicComplexity(program)) {
    supports_logeval.insert(id);
  }
  if (vireval.init(program)) {
    supports_vireval.insert(id);
  }
  if (with_loop) {
    has_loop.insert(id);
  }
  if (with_formula) {
    has_formula.insert(id);
  }
  blocks_collector.add(program);
}

void Stats::updateSequenceStats(UID id, bool program_found,
                                bool formula_found) {
  num_sequences++;
  num_programs += static_cast<int64_t>(program_found);
  num_formulas += static_cast<int64_t>(formula_found);
  if (program_found) {
    all_program_ids.insert(id);
  } else {
    all_program_ids.erase(id);
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
        SequenceProgram::collectLatestProgramIds(20, 200, 200);  // magic number
  }
}

int64_t Stats::getTransitiveLength(UID id) const {
  if (visited_programs.find(id) != visited_programs.end()) {
    visited_programs.clear();
    if (printed_recursion_warning.find(id) == printed_recursion_warning.end()) {
      printed_recursion_warning.insert(id);
      Log::get().debug("Recursion detected: " + id.string());
    }
    return -1;
  }
  visited_programs.insert(id);
  if (program_lengths.find(id) == program_lengths.end()) {
    Log::get().debug("Invalid reference: " + id.string());
    return -1;
  }
  int64_t length = program_lengths.at(id);
  auto range = call_graph.equal_range(id);
  for (auto &it = range.first; it != range.second; it++) {
    auto len = getTransitiveLength(it->second);
    if (len < 0) {
      length = -1;
      break;
    }
    length += len;
  }
  visited_programs.erase(id);
  return length;
}

size_t Stats::getNumUsages(UID id) const {
  auto it = program_usages.find(id);
  if (it != program_usages.end()) {
    return it->second;
  }
  return 0;
}

RandomProgramIds::RandomProgramIds(const UIDSet &ids) {
  ids_set = ids;
  for (auto id : ids) {
    ids_vector.push_back(id);
  }
}

bool RandomProgramIds::empty() const { return ids_set.empty(); }

bool RandomProgramIds::exists(UID id) const { return ids_set.exists(id); }

UID RandomProgramIds::get() const {
  if (!ids_vector.empty()) {
    return ids_vector[Random::get().gen() % ids_vector.size()];
  }
  return UID();
}

RandomProgramIds2::RandomProgramIds2(const Stats &stats)
    : all_program_ids(stats.all_program_ids),
      latest_program_ids(stats.latest_program_ids) {}

bool RandomProgramIds2::exists(UID id) const {
  return all_program_ids.exists(id) || latest_program_ids.exists(id);
}

UID RandomProgramIds2::get() const {
  if (Random::get().gen() % 2 == 0 ||
      latest_program_ids.empty()) {  // magic number
    return all_program_ids.get();
  } else {
    return latest_program_ids.get();
  }
}

UID RandomProgramIds2::getFromAll() const { return all_program_ids.get(); }
