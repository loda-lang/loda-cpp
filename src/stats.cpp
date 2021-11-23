#include "stats.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include "file.hpp"
#include "oeis_sequence.hpp"
#include "parser.hpp"
#include "program_util.hpp"

Stats::Stats()
    : num_programs(0),
      num_sequences(0),
      num_ops_per_type(Operation::Types.size(), 0) {}

void Stats::load(std::string path) {
  ensureTrailingSlash(path);
  Log::get().debug("Loading program stats from " + path);

  const std::string sep(",");
  std::string full, line, k, v, w, l;
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
    found_programs.resize(100000, false);
    cached_b_files.resize(100000, false);
    program_lengths.resize(100000, false);
    int64_t largest_id = 0;
    while (std::getline(programs, line)) {
      std::stringstream s(line);
      std::getline(s, k, ',');
      std::getline(s, v, ',');
      std::getline(s, w, ',');
      std::getline(s, l);
      int64_t id = std::stoll(k);
      largest_id = std::max<int64_t>(largest_id, id);
      if ((size_t)id >= found_programs.size()) {
        size_t new_size = std::max<size_t>(found_programs.size() * 2, id + 1);
        found_programs.resize(new_size);
        cached_b_files.resize(new_size);
        program_lengths.resize(new_size);
      }
      found_programs[id] = std::stoll(v);
      cached_b_files[id] = std::stoll(w);
      program_lengths[id] = std::stoll(l);
    }
    found_programs.resize(largest_id + 1);
    cached_b_files.resize(largest_id + 1);
    program_lengths.resize(largest_id + 1);
    programs.close();
  }

  {
    full = path + "call_graph.csv";
    Log::get().debug("Loading " + full);
    std::ifstream call(full);
    if (!std::getline(call, line) || line != "caller,callee") {
      throw std::runtime_error("Unexpected first line in " + full);
    }
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
    if (!std::getline(summary, line) || line != "num_programs,num_sequences") {
      throw std::runtime_error("Unexpected first line in " + full);
    }
    std::getline(summary, k, ',');
    std::getline(summary, v);
    num_programs = std::stoll(k);
    num_sequences = std::stoll(v);
  }

  { blocks.load(path + "blocks.asm"); }

  // TODO: remaining stats

  Log::get().debug("Finished loading program stats");
}

void Stats::save(std::string path) {
  ensureTrailingSlash(path);
  Log::get().debug("Saving program stats to " + path);

  const std::string sep(",");
  std::ofstream constants(path + "constant_counts.csv");
  for (auto &e : num_constants) {
    constants << e.first << sep << e.second << "\n";
  }
  constants.close();
  std::ofstream programs(path + "programs.csv");
  for (size_t i = 0; i < found_programs.size(); i++) {
    const bool f = found_programs[i];
    const bool b = cached_b_files[i];
    const int64_t l = program_lengths[i];
    if (f || b || l) {
      programs << i << sep << f << sep << b << sep << l << "\n";
    }
  }
  programs.close();
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
  summary << "num_programs,num_sequences\n";
  summary << num_programs << sep << num_sequences << "\n";
  summary.close();

  std::ofstream cal(path + "call_graph.csv");
  cal << "caller,callee\n";
  for (auto it : call_graph) {
    cal << OeisSequence(it.first).id_str() << sep
        << OeisSequence(it.second).id_str() << "\n";
  }
  cal.close();

  if (steps.total)  // write steps stats only if present
  {
    std::ofstream steps_out(path + "steps.csv");
    steps_out << "total,min,max,runs\n";
    steps_out << steps.total << sep << steps.min << sep << steps.max << sep
              << steps.runs << "\n";
    steps_out.close();
  }

  { blocks.save(path + "blocks.asm"); }

  Log::get().debug("Finished saving program stats");
}

std::string Stats::getMainStatsFile(std::string path) const {
  ensureTrailingSlash(path);
  path += "constant_counts.csv";
  return path;
}

void Stats::updateProgramStats(size_t id, const Program &program) {
  num_programs++;
  const size_t num_ops = ProgramUtil::numOps(program, false);
  if (id >= found_programs.size()) {
    const size_t new_size = std::max<size_t>(id + 1, 2 * found_programs.size());
    program_lengths.resize(new_size);
  }
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
    if (Operation::Metadata::get(op.type).num_operands == 2 &&
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
      call_graph.insert(
          std::pair<int64_t, int64_t>(id, op.source.value.asInt()));
    }
    o.pos++;
  }
  blocks_collector.add(program);
}

void Stats::updateSequenceStats(size_t id, bool program_found,
                                bool has_b_file) {
  num_sequences++;
  if (id >= found_programs.size()) {
    const size_t new_size = std::max<size_t>(id + 1, 2 * found_programs.size());
    found_programs.resize(new_size);
    cached_b_files.resize(new_size);
    program_lengths.resize(new_size);
  }
  found_programs[id] = program_found;
  cached_b_files[id] = has_b_file;
}

void Stats::finalize() {
  if (!blocks_collector.empty()) {
    if (!blocks.list.ops.empty()) {
      Log::get().error("Attempted overwrite of blocks stats", true);
    }
    blocks = blocks_collector.finalize();
  }
}

int64_t Stats::getTransitiveLength(size_t id) const {
  if (visited_programs.find(id) != visited_programs.end()) {
    visited_programs.clear();
    if (printed_recursion_warning.find(id) == printed_recursion_warning.end()) {
      printed_recursion_warning.insert(id);
      Log::get().warn("Recursion detected in stats for " +
                      OeisSequence(id).getProgramPath());
    }
    return -1;
  }
  visited_programs.insert(id);
  int64_t length = program_lengths.at(id);
  auto range = call_graph.equal_range(id);
  for (auto &it = range.first; it != range.second; it++) {
    length += getTransitiveLength(it->second);
  }
  visited_programs.erase(id);
  return length;
}
