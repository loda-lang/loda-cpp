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
#include "sys/csv.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

const std::string Stats::CALL_GRAPH_HEADER("caller,callee");
const std::string Stats::PROGRAMS_HEADER(
    "id,submitter,length,usages,inc_eval,log_eval,vir_eval,loop,formula,"
    "indirect,ops_mask");
const std::string Stats::STEPS_HEADER("total,min,max,runs");
const std::string Stats::SUMMARY_HEADER(
    "num_sequences,num_programs,num_formulas");
const std::string SUBMITTERS_HEADER = "submitter,ref_id,num_programs";
const std::string OPERATION_TYPES_HEADER = "name,ref_id,count";

Stats::Stats()
    : num_programs(0),
      num_sequences(0),
      num_formulas(0),
      num_ops_per_type(Operation::Types.size(), 0) {}

void Stats::load(std::string path) {
  ensureTrailingFileSep(path);
  Log::get().debug("Loading program stats from " + path);
  auto start_time = std::chrono::steady_clock::now();

  std::string full;
  Parser parser;
  Operation op;
  Operand count;

  {
    full = path + "constant_counts.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    while (reader.readRow()) {
      num_constants[Number(reader.getField(0))] = reader.getIntegerField(1);
    }
    reader.close();
  }

  {
    full = path + "program_lengths.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    while (reader.readRow()) {
      auto l = reader.getIntegerField(0);
      while (l >= (int64_t)num_programs_per_length.size()) {
        num_programs_per_length.push_back(0);
      }
      num_programs_per_length[l] = reader.getIntegerField(1);
    }
    reader.close();
  }

  {
    full = path + "operation_types.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    reader.checkHeader(OPERATION_TYPES_HEADER);
    while (reader.readRow()) {
      auto type = Operation::Metadata::get(reader.getField(0)).type;
      // Field 1 is ref_id, which we don't need to load (it's in metadata)
      num_ops_per_type.at(static_cast<size_t>(type)) =
          reader.getIntegerField(2);
    }
    reader.close();
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
    CsvReader reader(full);
    int64_t largest_id = 0;
    reader.checkHeader(PROGRAMS_HEADER);
    while (reader.readRow()) {
      UID id(reader.getField(0));
      largest_id = std::max<int64_t>(largest_id, id.number());
      all_program_ids.insert(id);
      program_submitter[id] = reader.getIntegerField(1);
      program_lengths[id] = reader.getIntegerField(2);
      program_usages[id] = reader.getIntegerField(3);
      if (reader.getIntegerField(4)) {
        supports_inceval.insert(id);
      }
      if (reader.getIntegerField(5)) {
        supports_logeval.insert(id);
      }
      if (reader.getIntegerField(6)) {
        supports_vireval.insert(id);
      }
      if (reader.getIntegerField(7)) {
        has_loop.insert(id);
      }
      if (reader.getIntegerField(8)) {
        has_formula.insert(id);
      }
      if (reader.getIntegerField(9)) {
        has_indirect.insert(id);
      }
      program_operation_types_bitmask[id] = reader.getIntegerField(10);
    }
    reader.close();
  }

  {
    full = path + "latest_programs.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    latest_program_ids.clear();
    while (reader.readRow()) {
      latest_program_ids.insert(UID(reader.getField(0)));
    }
    reader.close();
  }

  {
    full = path + "call_graph.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    reader.checkHeader(CALL_GRAPH_HEADER);
    call_graph.clear();
    while (reader.readRow()) {
      call_graph.emplace(UID(reader.getField(0)), UID(reader.getField(1)));
    }
    reader.close();
  }

  {
    full = path + "summary.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    reader.checkHeader(SUMMARY_HEADER);
    if (reader.readRow()) {
      num_sequences = reader.getIntegerField(0);
      num_programs = reader.getIntegerField(1);
      num_formulas = reader.getIntegerField(2);
    }
  }

  {
    blocks.load(path + "blocks.asm");
  }

  {
    full = path + "submitters.csv";
    Log::get().debug("Loading " + full);
    CsvReader reader(full);
    num_programs_per_submitter.clear();
    reader.checkHeader(SUBMITTERS_HEADER);
    while (reader.readRow()) {
      auto ref_id = reader.getIntegerField(1);
      submitter_ref_ids[reader.getField(0)] = ref_id;
      if (ref_id >= static_cast<int64_t>(num_programs_per_submitter.size())) {
        num_programs_per_submitter.resize(ref_id + 1);
      }
      num_programs_per_submitter[ref_id] = reader.getIntegerField(2);
    }
    reader.close();
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

  {
    CsvWriter writer(path + "constant_counts.csv");
    for (auto& e : num_constants) {
      writer.writeRow(e.first.to_string(), std::to_string(e.second));
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "programs.csv");
    writer.writeHeader(PROGRAMS_HEADER);
    for (auto id : all_program_ids) {
      const auto inceval = supports_inceval.exists(id);
      const auto logeval = supports_logeval.exists(id);
      const auto vireval = supports_vireval.exists(id);
      const auto loop_flag = has_loop.exists(id);
      const auto formula_flag = has_formula.exists(id);
      const auto indirect_flag = has_indirect.exists(id);
      writer.writeRow({id.string(), std::to_string(program_submitter[id]),
                       std::to_string(program_lengths[id]),
                       std::to_string(program_usages[id]),
                       std::to_string(inceval), std::to_string(logeval),
                       std::to_string(vireval), std::to_string(loop_flag),
                       std::to_string(formula_flag),
                       std::to_string(indirect_flag),
                       std::to_string(program_operation_types_bitmask[id])});
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "latest_programs.csv");
    for (auto id : latest_program_ids) {
      writer.writeRow({id.string()});
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "program_lengths.csv");
    for (size_t i = 0; i < num_programs_per_length.size(); i++) {
      if (num_programs_per_length[i] > 0) {
        writer.writeRow(std::to_string(i),
                        std::to_string(num_programs_per_length[i]));
      }
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "operation_types.csv");
    writer.writeHeader(OPERATION_TYPES_HEADER);
    for (size_t i = 0; i < num_ops_per_type.size(); i++) {
      if (num_ops_per_type[i] > 0) {
        const auto& meta =
            Operation::Metadata::get(static_cast<Operation::Type>(i));
        writer.writeRow(meta.name, std::to_string(meta.ref_id),
                        std::to_string(num_ops_per_type[i]));
      }
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "operation_counts.csv");
    for (auto& op : num_operations) {
      const auto& meta = Operation::Metadata::get(op.first.type);
      writer.writeRow({meta.name, ProgramUtil::operandToString(op.first.target),
                       ProgramUtil::operandToString(op.first.source),
                       std::to_string(op.second)});
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "operation_pos_counts.csv");
    for (auto& o : num_operation_positions) {
      const auto& meta = Operation::Metadata::get(o.first.op.type);
      writer.writeRow({std::to_string(o.first.pos), std::to_string(o.first.len),
                       meta.name,
                       ProgramUtil::operandToString(o.first.op.target),
                       ProgramUtil::operandToString(o.first.op.source),
                       std::to_string(o.second)});
    }
    writer.close();
  }

  {
    CsvWriter writer(path + "summary.csv");
    writer.writeHeader(SUMMARY_HEADER);
    writer.writeRow({std::to_string(num_sequences),
                     std::to_string(num_programs),
                     std::to_string(num_formulas)});
    writer.close();
  }

  {
    CsvWriter writer(path + "call_graph.csv");
    writer.writeHeader(CALL_GRAPH_HEADER);
    for (auto it : call_graph) {
      writer.writeRow(it.first.string(), it.second.string());
    }
    writer.close();
  }

  if (steps.total) {  // write steps stats only if present
    CsvWriter writer(path + "steps.csv");
    writer.writeHeader(STEPS_HEADER);
    writer.writeRow({std::to_string(steps.total), std::to_string(steps.min),
                     std::to_string(steps.max), std::to_string(steps.runs)});
    writer.close();
  }

  {
    blocks.save(path + "blocks.asm");
  }

  {
    CsvWriter writer(path + "submitters.csv");
    writer.writeHeader(SUBMITTERS_HEADER);
    for (const auto& e : submitter_ref_ids) {
      writer.writeRow({e.first, std::to_string(e.second),
                       std::to_string(num_programs_per_submitter[e.second])});
    }
    writer.close();
  }

  Log::get().debug("Finished saving program stats");
}

std::string Stats::getMainStatsFile(std::string path) const {
  ensureTrailingFileSep(path);
  path += "constant_counts.csv";
  return path;
}

void Stats::updateProgramStats(UID id, const Program& program,
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
  bool with_indirect = false;
  int64_t ops_bitmask = 0;
  for (auto& op : program.ops) {
    num_ops_per_type[static_cast<size_t>(op.type)]++;
    // Set the bit corresponding to this operation type's ref_id
    const auto& meta = Operation::Metadata::get(op.type);
    ops_bitmask |= (1LL << meta.ref_id);
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
    if (ProgramUtil::hasIndirectOperand(op)) {
      with_indirect = true;
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
  if (with_indirect) {
    has_indirect.insert(id);
  }
  program_operation_types_bitmask[id] = ops_bitmask;
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
    latest_program_ids = SequenceProgram::collectLatestProgramIds(
        Setup::NUM_COMMITS_FOR_PROGRAMS, 200, 200);  // magic number
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
  for (auto& it = range.first; it != range.second; it++) {
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

RandomProgramIds::RandomProgramIds(const UIDSet& ids) {
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

RandomProgramIds2::RandomProgramIds2(const Stats& stats)
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
