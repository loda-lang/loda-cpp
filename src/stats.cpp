#include "stats.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include "file.hpp"
#include "oeis_sequence.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"

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
    all_program_ids.resize(100000, false);
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
      if ((size_t)id >= all_program_ids.size()) {
        size_t new_size = std::max<size_t>(all_program_ids.size() * 2, id + 1);
        all_program_ids.resize(new_size);
        cached_b_files.resize(new_size);
        program_lengths.resize(new_size);
      }
      all_program_ids[id] = std::stoll(v);
      cached_b_files[id] = std::stoll(w);
      program_lengths[id] = std::stoll(l);
    }
    all_program_ids.resize(largest_id + 1);
    cached_b_files.resize(largest_id + 1);
    program_lengths.resize(largest_id + 1);
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
        throw std::runtime_error("Unexpected latest program ID: " +
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
  for (size_t i = 0; i < all_program_ids.size(); i++) {
    const bool f = all_program_ids[i];
    const bool b = cached_b_files[i];
    const int64_t l = program_lengths[i];
    if (f || b || l) {
      programs << i << sep << f << sep << b << sep << l << "\n";
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

  if (steps.total) {  // write steps stats only if present
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
  if (id >= all_program_ids.size()) {
    const size_t new_size =
        std::max<size_t>(id + 1, 2 * all_program_ids.size());
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
  if (id >= all_program_ids.size()) {
    const size_t new_size =
        std::max<size_t>(id + 1, 2 * all_program_ids.size());
    all_program_ids.resize(new_size);
    cached_b_files.resize(new_size);
    program_lengths.resize(new_size);
  }
  all_program_ids[id] = program_found;
  cached_b_files[id] = has_b_file;
}

void Stats::finalize() {
  if (!blocks_collector.empty()) {
    if (!blocks.list.ops.empty()) {
      Log::get().error("Attempted overwrite of blocks stats", true);
    }
    blocks = blocks_collector.finalize();
  }
  if (latest_program_ids.empty()) {
    collectLatestProgramIds();
  }
}

void Stats::collectLatestProgramIds() {
  auto progs_dir = Setup::getProgramsHome();
  static const size_t max_commits = 100;            // magic number
  static const size_t max_added_programs = 500;     // magic number
  static const size_t max_modified_programs = 500;  // magic number
  if (!isDir(progs_dir + ".git")) {
    Log::get().warn(
        "Cannot read commit history because the .git folder was not found");
    return;
  }
  const std::string git_tmp = getTmpDir() + "loda_git_tmp.txt";
  git(progs_dir, "log --oneline --format=\"%H\" -n " +
                     std::to_string(max_commits) + " > \"" + git_tmp + "\"");
  std::string line;
  std::vector<std::string> commits;
  {
    // read commits from file
    std::ifstream in(git_tmp);
    while (std::getline(in, line)) {
      if (!line.empty()) {
        commits.push_back(line);
      }
    }
  }
  std::remove(git_tmp.c_str());
  if (commits.empty()) {
    Log::get().warn("Cannot read programs commit history");
    return;
  }
  std::set<int64_t> ids;
  size_t num_added_ids = 0, num_modified_ids = 0;
  for (const auto &commit : commits) {
    if (num_added_ids >= max_added_programs &&
        num_modified_ids >= max_modified_programs) {
      break;
    }
    git(progs_dir, "diff-tree --no-commit-id --name-status -r " + commit +
                       " > \"" + git_tmp + "\"");
    {
      // read changed file names from file
      std::ifstream in(git_tmp);
      std::string status, path;
      while (std::getline(in, line)) {
        std::istringstream iss(line);
        iss >> status;
        iss >> path;
        if (path.size() >= 11 && path.substr(path.size() - 4) == ".asm") {
          auto id_str = path.substr(path.size() - 11, 7);
          try {
            OeisSequence seq(id_str);
            if (isFile(seq.getProgramPath())) {
              if (status == "A" && num_added_ids < max_added_programs) {
                ids.insert(seq.id);
                num_added_ids++;
              } else if (status == "M" &&
                         num_modified_ids < max_modified_programs) {
                ids.insert(seq.id);
                num_modified_ids++;
              }
            }
          } catch (const std::exception &) {
            // ignore because it is not a program of an OEIS sequence
          }
        }
      }
    }
    std::remove(git_tmp.c_str());
  }
  latest_program_ids.clear();
  for (auto id : ids) {
    if (id >= static_cast<int64_t>(latest_program_ids.size())) {
      const size_t new_size =
          std::max<size_t>(id + 1, 2 * latest_program_ids.size());
      latest_program_ids.resize(new_size);
    }
    latest_program_ids[id] = true;
  }
  if (latest_program_ids.empty()) {
    Log::get().warn("Cannot read programs commit history");
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
