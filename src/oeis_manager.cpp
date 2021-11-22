#include "oeis_manager.hpp"

#include <stdlib.h>
#include <time.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

#include "config.hpp"
#include "file.hpp"
#include "interpreter.hpp"
#include "number.hpp"
#include "oeis_list.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "stats.hpp"
#include "util.hpp"
#include "web_client.hpp"

void throwParseError(const std::string &line) {
  Log::get().error("error parsing OEIS line: " + line, true);
}

OeisManager::OeisManager(const Settings &settings, bool force_overwrite,
                         const std::string &stats_home)
    : settings(settings),
      overwrite_mode(force_overwrite
                         ? OverwriteMode::ALL
                         : ConfigLoader::load(settings).overwrite_mode),
      evaluator(settings),
      finder(settings),
      finder_initialized(false),
      optimizer(settings),
      loaded_count(0),
      total_count(0),
      stats_loaded(false),
      stats_home(stats_home.empty() ? (Setup::getLodaHome() + "stats")
                                    : stats_home)  // no trailing / here
{}

void OeisManager::load() {
  // check if already loaded
  if (total_count > 0) {
    return;
  }

  // first load the deny and protect lists (needs no lock)
  const std::string oeis_dir = Setup::getProgramsHome() + "oeis" + FILE_SEP;
  OeisList::loadList(oeis_dir + "deny.txt", deny_list);
  OeisList::loadList(oeis_dir + "overwrite.txt", overwrite_list);
  OeisList::loadList(oeis_dir + "protect.txt", protect_list);

  // load invalid matches map
  const std::string invalid_matches_file =
      OeisList::getListsHome() + "invalid_matches.txt";
  try {
    OeisList::loadMap(invalid_matches_file, invalid_matches_map);
  } catch (const std::exception &) {
    Log::get().warn("Resetting corrupt file " + invalid_matches_file);
    invalid_matches_map.clear();
    std::remove(invalid_matches_file.c_str());
  }

  std::chrono::steady_clock::time_point start_time;
  {
    // obtain lock
    FolderLock lock(Setup::getOeisHome());

    // update index if needed
    update();

    // load sequence data, names and deny list
    Log::get().info("Loading sequences from the OEIS index");
    start_time = std::chrono::steady_clock::now();
    loadData();
    loadNames();

    // lock released at the end of this block
  }

  // shrink sequences vector again
  if (!sequences.empty()) {
    size_t i;
    for (i = sequences.size() - 1; i > 0; i--) {
      if (sequences[i].id != 0) {
        break;
      }
    }
    sequences.resize(i + 1);
  }

  // print summary
  auto cur_time = std::chrono::steady_clock::now();
  double duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        cur_time - start_time)
                        .count() /
                    1000.0;
  std::stringstream buf;
  buf.setf(std::ios::fixed);
  buf.precision(2);
  buf << duration;
  Log::get().info("Loaded " + std::to_string(loaded_count) + "/" +
                  std::to_string(total_count) + " sequences in " + buf.str() +
                  "s");
}

void OeisManager::loadData() {
  std::string path = Setup::getOeisHome() + "stripped";
  std::ifstream stripped(path);
  if (!stripped.good()) {
    Log::get().error("OEIS data not found: " + path, true);
  }
  std::string line;
  std::string buf;
  size_t pos;
  size_t id;
  Sequence seq_full, seq_big;

  loaded_count = 0;
  total_count = 0;

  while (std::getline(stripped, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != 'A') {
      throwParseError(line);
    }
    total_count++;
    pos = 1;
    id = 0;
    for (pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9';
         ++pos) {
      id = (10 * id) + (line[pos] - '0');
    }
    if (pos >= line.length() || line[pos] != ' ' || id == 0) {
      throwParseError(line);
    }
    ++pos;
    if (pos >= line.length() || line[pos] != ',') {
      throwParseError(line);
    }
    ++pos;
    buf.clear();
    seq_full.clear();
    while (pos < line.length()) {
      if (line[pos] == ',') {
        Number num(buf);
        if (OeisSequence::isTooBig(num)) {
          break;
        }
        seq_full.push_back(num);
        buf.clear();
      } else if ((line[pos] >= '0' && line[pos] <= '9') || line[pos] == '-') {
        buf += line[pos];
      } else {
        throwParseError(line);
      }
      ++pos;
    }

    // check minimum number of terms
    if (seq_full.size() < settings.num_terms) {
      continue;
    }

    // add sequence to index
    if (id >= sequences.size()) {
      sequences.resize(2 * id);
    }
    sequences[id] = OeisSequence(id, "", seq_full);
    loaded_count++;
  }
}

void OeisManager::loadNames() {
  Log::get().debug("Loading sequence names from the OEIS index");
  std::string path = Setup::getOeisHome() + "names";
  std::ifstream names(path);
  if (!names.good()) {
    Log::get().error("OEIS data not found: " + path, true);
  }
  std::string line;
  size_t pos;
  size_t id;
  while (std::getline(names, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != 'A') {
      throwParseError(line);
    }
    pos = 1;
    id = 0;
    for (pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9';
         ++pos) {
      id = (10 * id) + (line[pos] - '0');
    }
    if (pos >= line.length() || line[pos] != ' ' || id == 0) {
      throwParseError(line);
    }
    ++pos;
    if (id < sequences.size() && sequences[id].id == id) {
      sequences[id].name = line.substr(pos);
      if (Log::get().level == Log::Level::DEBUG) {
        std::stringstream buf;
        buf << "Loaded sequence " << sequences[id];
        Log::get().debug(buf.str());
      }
    }
  }
}

Finder &OeisManager::getFinder() {
  if (!finder_initialized) {
    // generate stats is needed
    getStats();

    ignore_list.clear();
    for (auto &seq : sequences) {
      if (seq.id == 0) {
        continue;
      }
      if (shouldMatch(seq)) {
        auto seq_norm = seq.getTerms(settings.num_terms);
        finder.insert(seq_norm, seq.id);
      } else {
        ignore_list.insert(seq.id);
      }
    }
    finder_initialized = true;

    // print summary
    Log::get().info("Initialized " +
                    std::to_string(finder.getMatchers().size()) +
                    " matchers (ignoring " +
                    std::to_string(ignore_list.size()) + " sequences)");
    finder.logSummary(loaded_count);
  }
  return finder;
}

bool OeisManager::shouldMatch(const OeisSequence &seq) const {
  if (seq.id == 0) {
    return false;
  }

  // sequence on the deny or protect list?
  if (deny_list.find(seq.id) != deny_list.end()) {
    return false;
  }
  if (protect_list.find(seq.id) != protect_list.end()) {
    return false;
  }

  // too many invalid matches already?
  bool too_many_matches = false;
  auto it = invalid_matches_map.find(seq.id);
  if (it != invalid_matches_map.end() && it->second > 0 &&
      (Random::get().gen() % it->second) >= 100)  // magic number
  {
    too_many_matches = true;
  }

  // check if program exists
  const bool prog_exists =
      (seq.id < stats.found_programs.size()) && stats.found_programs[seq.id];

  // decide based on overwrite mode
  switch (overwrite_mode) {
    case OverwriteMode::NONE:
      return !prog_exists && !too_many_matches;

    case OverwriteMode::ALL:
      return true;

    case OverwriteMode::AUTO: {
      if (too_many_matches) {
        return false;
      }
      if (!prog_exists) {
        return true;
      }
      const bool should_overwrite =
          overwrite_list.find(seq.id) != overwrite_list.end();
      const bool is_complex =
          stats.getTransitiveLength(seq.id) > 10;  // magic number
      return is_complex || should_overwrite;
    }
  }
  return true;  // unreachable
}

void OeisManager::update() {
  std::vector<std::string> files = {"stripped", "names"};

  // check which files need to be updated
  auto it = files.begin();
  int64_t age_in_days = -1;
  while (it != files.end()) {
    auto path = Setup::getOeisHome() + *it;
    age_in_days = getFileAgeInDays(path);
    if (age_in_days >= 0 && age_in_days < Setup::getUpdateIntervalInDays()) {
      // no need to update this file
      it = files.erase(it);
      continue;
    }
    it++;
  }
  if (!files.empty()) {
    Setup::checkLatestedVersion();
    if (age_in_days == -1) {
      Log::get().info("Creating OEIS index at " + Setup::getOeisHome());
      ensureDir(Setup::getOeisHome());
    } else {
      Log::get().info("Updating OEIS index (last update " +
                      std::to_string(age_in_days) + " days ago)");
    }
    std::string cmd, path;
    for (auto &file : files) {
      path = Setup::getOeisHome() + file;
      WebClient::get("https://oeis.org/" + file + ".gz", path + ".gz");
      std::ifstream f(Setup::getOeisHome() + file);
      if (f.good()) {
        f.close();
        std::remove(path.c_str());
      }
      gunzip(path + ".gz");
    }
    // update programs repository using git pull
    auto mode = Setup::getMiningMode();
    auto progs_dir = Setup::getProgramsHome();
    if (mode != MINING_MODE_SERVER && hasGit() && isDir(progs_dir + ".git")) {
      Log::get().info("Updating programs repository");
      std::string git_cmd =
          "cd " + progs_dir + "&& git pull origin main -q --ff-only";
      if (system(git_cmd.c_str()) != 0) {
        Log::get().warn(
            "Update of programs repository failed; please update it manually.");
      }
    }
  }
}

void OeisManager::generateStats(int64_t age_in_days) {
  load();
  std::string msg;
  if (age_in_days < 0) {
    msg = "Generating program stats at " + stats_home;
  } else {
    msg = "Regenerating program stats (last update " +
          std::to_string(age_in_days) + " days ago)";
  }
  Log::get().info(msg);
  stats = Stats();

  size_t num_processed = 0;
  Parser parser;
  Program program;
  std::string file_name;
  bool has_b_file, has_program;

  for (auto &s : sequences) {
    if (s.id == 0) {
      continue;
    }
    file_name = s.getProgramPath();
    std::ifstream program_file(file_name);
    std::ifstream b_file(s.getBFilePath());
    has_b_file = b_file.good();
    has_program = false;
    if (program_file.good()) {
      try {
        program = parser.parse(program_file);
        has_program = true;
      } catch (const std::exception &exc) {
        Log::get().error(
            "Error parsing " + file_name + ": " + std::string(exc.what()),
            false);
        continue;
      }

      ProgramUtil::removeOps(program, Operation::Type::NOP);

      // update stats
      stats.updateProgramStats(s.id, program);
      num_processed++;
    }
    stats.updateSequenceStats(s.id, has_program, has_b_file);
  }

  // write stats
  stats.finalize();
  stats.save(stats_home);

  // done
  Log::get().info("Finished stats generation for " +
                  std::to_string(num_processed) + " programs");
}

void OeisManager::migrate() {
  for (size_t id = 0; id < 400000; id++) {
    OeisSequence s(id);
    std::ifstream f(s.getProgramPath());
    Parser parser;
    Program p, out;
    if (f.good()) {
      Log::get().warn("Migrating " + s.getProgramPath());
      p = parser.parse(f);
      f.close();
      ProgramUtil::migrateOutputCell(p, 1, 0);
      for (size_t i = 0; i < p.ops.size(); i++) {
        if (p.ops[i].type != Operation::Type::NOP) {
          p.ops.insert(p.ops.begin() + i, Operation());
          break;
        }
      }
      std::ofstream out(s.getProgramPath());
      ProgramUtil::print(p, out);
      out.close();
    }
  }
}

const std::vector<OeisSequence> &OeisManager::getSequences() const {
  return sequences;
}

const Stats &OeisManager::getStats() {
  if (!stats_loaded) {
    // obtain lock
    FolderLock lock(stats_home);

    // check age of stats
    auto age_in_days = getFileAgeInDays(stats.getMainStatsFile(stats_home));
    if (age_in_days < 0 || age_in_days >= Setup::getUpdateIntervalInDays()) {
      generateStats(age_in_days);
    }
    try {
      stats.load(stats_home);
    } catch (const std::exception &e) {
      Log::get().warn("Exception during stats loading, regenerating...");
      generateStats(age_in_days);
      stats.load(stats_home);  // reload
    }
    stats_loaded = true;

    // lock released at the end of this block
  }

  // publish metrics
  std::vector<Metrics::Entry> entries;
  std::map<std::string, std::string> labels;
  labels["kind"] = "total";
  entries.push_back({"programs", labels, (double)stats.num_programs});
  entries.push_back({"sequences", labels, (double)total_count});
  labels["kind"] = "used";
  entries.push_back({"sequences", labels, (double)stats.num_sequences});
  labels.clear();
  for (size_t i = 0; i < stats.num_ops_per_type.size(); i++) {
    if (stats.num_ops_per_type[i] > 0) {
      labels["type"] =
          Operation::Metadata::get(static_cast<Operation::Type>(i)).name;
      entries.push_back(
          {"operation_types", labels, (double)stats.num_ops_per_type[i]});
    }
  }
  Metrics::get().write(entries);

  return stats;
}

void OeisManager::addSeqComments(Program &p) const {
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::SEQ &&
        op.source.type == Operand::Type::CONSTANT) {
      auto id = op.source.value.asInt();
      if (id >= 0 && id < static_cast<int64_t>(sequences.size()) &&
          sequences[id].id) {
        op.comment = sequences[id].name;
      }
    }
  }
}

void OeisManager::dumpProgram(size_t id, Program &p, const std::string &file,
                              const std::string &submitted_by) const {
  ProgramUtil::removeOps(p, Operation::Type::NOP);
  ProgramUtil::removeComments(p);
  addSeqComments(p);
  ensureDir(file);
  auto &seq = sequences.at(id);
  Program tmp;
  Operation nop(Operation::Type::NOP);
  nop.comment = seq.to_string();
  tmp.ops.push_back(nop);
  if (!submitted_by.empty()) {
    nop.comment = ProgramUtil::SUBMITTED_BY_PREFIX + " " + submitted_by;
    tmp.ops.push_back(nop);
  }
  nop.comment = seq.getTerms(OeisSequence::DEFAULT_SEQ_LENGTH).to_string();
  static constexpr size_t max_length = 500;
  if (nop.comment.size() > max_length) {  // magic number
    nop.comment = nop.comment.substr(0, max_length);
    auto n = nop.comment.find_last_of(',');
    if (n != std::string::npos) {
      nop.comment = nop.comment.substr(0, n);
    }
  }
  tmp.ops.push_back(nop);
  nop.comment.clear();
  tmp.ops.push_back(nop);
  p.ops.insert(p.ops.begin(), tmp.ops.begin(), tmp.ops.end());
  std::ofstream out(file);
  ProgramUtil::print(p, out);
  out.close();
}

void OeisManager::alert(Program p, size_t id, const std::string &prefix,
                        const std::string &color,
                        const std::string &submitted_by) const {
  auto &seq = sequences.at(id);
  std::stringstream buf;
  buf << prefix << " program for " << seq
      << " Terms: " << seq.getTerms(settings.num_terms);
  if (!submitted_by.empty()) {
    buf << ". " << ProgramUtil::SUBMITTED_BY_PREFIX << " " << submitted_by;
  }
  auto msg = buf.str();
  Log::AlertDetails details;
  details.title = seq.id_str();
  details.title_link = seq.url_str();
  details.color = color;
  buf << "\\n\\`\\`\\`\\n";
  ProgramUtil::removeOps(p, Operation::Type::NOP);
  addSeqComments(p);
  ProgramUtil::print(p, buf, "\\n");
  buf << "\\`\\`\\`";
  details.text = buf.str();
  Log::get().alert(msg, details);
}

size_t getBadOpsCount(const Program &p) {
  // we prefer programs the following programs:
  // - w/o indirect memory access
  // - w/o loops that have non-constant args
  // - w/o gcd with powers of a small constant
  size_t num_ops = ProgramUtil::numOps(p, Operand::Type::INDIRECT);
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::LPB &&
        op.source.type != Operand::Type::CONSTANT) {
      num_ops++;
    }
    if (op.type == Operation::Type::GCD &&
        op.source.type == Operand::Type::CONSTANT &&
        Minimizer::getPowerOf(op.source.value) != 0) {
      num_ops++;
    }
  }
  return num_ops;
}

std::string OeisManager::isOptimizedBetter(Program existing, Program optimized,
                                           const OeisSequence &seq) {
  // check if there are illegal recursions; do we really need this?!
  for (auto &op : optimized.ops) {
    if (op.type == Operation::Type::SEQ) {
      if (op.source.type != Operand::Type::CONSTANT ||
          op.source.value == Number(seq.id)) {
        return "";
      }
    }
  }

  // remove nops...
  optimizer.removeNops(existing);
  optimizer.removeNops(optimized);

  // we want at least one operation (avoid empty program for A000004
  if (optimized.ops.empty()) {
    return "";
  }

  // if the programs are the same, no need to evaluate them
  if (optimized == existing) {
    return "";
  }

  // check if there are loops with contant number of iterations involved
  const int64_t const_loops_existing =
      ProgramUtil::hasLoopWithConstantNumIterations(existing);
  const int64_t const_loops_optimized =
      ProgramUtil::hasLoopWithConstantNumIterations(optimized);
  if (const_loops_optimized < const_loops_existing) {
    return "Better";
  } else if (const_loops_optimized > const_loops_existing) {
    return "";  // optimized is worse
  }

  // get extended sequence
  auto terms = seq.getTerms(OeisSequence::EXTENDED_SEQ_LENGTH);
  if (terms.empty()) {
    Log::get().error("Error fetching b-file for " + seq.id_str(), true);
  }

  // check if the first non-decreasing term is beyond the know sequence terms
  static const int64_t num_terms = 10000;  // magic number
  Sequence tmp;
  const auto optimized_steps = evaluator.eval(optimized, tmp, num_terms, false);
  if (tmp.get_first_non_decreasing_term() >=
      static_cast<int64_t>(terms.size())) {
    return "";
  }

  // compare number of successfully computed terms
  const auto existing_steps = evaluator.eval(existing, tmp, num_terms, false);
  if (optimized_steps.runs > existing_steps.runs) {
    return "Better";
  } else if (optimized_steps.runs < existing_steps.runs) {
    return "";  // optimized is worse
  }

  // compare number of "bad" operations
  auto optimized_bad_count = getBadOpsCount(optimized);
  auto existing_bad_count = getBadOpsCount(existing);
  if (optimized_bad_count < existing_bad_count) {
    return "Simpler";
  } else if (optimized_bad_count > existing_bad_count) {
    return "";  // optimized is worse
  }

  // ...and compare number of execution cycles
  if (optimized_steps.total < existing_steps.total) {
    return "Faster";
  } else if (optimized_steps.total > existing_steps.total) {
    return "";  // optimized is worse
  }

  return "";
}

update_program_result_t OeisManager::updateProgram(size_t id,
                                                   const Program &p) {
  auto &seq = sequences.at(id);
  const std::string global_file = seq.getProgramPath(false);
  const std::string local_file = seq.getProgramPath(true);
  const std::string submitted_by = ProgramUtil::getSubmittedBy(p);
  bool is_new = true;
  std::string change;
  update_program_result_t result;
  result.updated = false;
  result.is_new = false;

  // minimize and check the program
  auto minimized = finder.checkAndMinimize(p, seq);
  if (!minimized.first) {
    return result;
  }

  // check if there is an existing program already
  const bool has_global = isFile(global_file);
  const bool has_local = isFile(local_file);
  if (has_global || has_local) {
    std::string file_name = has_local ? local_file : global_file;
    std::ifstream in(file_name);
    if (ignore_list.find(id) == ignore_list.end()) {
      is_new = false;
      Parser parser;
      Program existing;
      try {
        existing = parser.parse(in);
      } catch (const std::exception &) {
        Log::get().error("Error parsing " + file_name, false);
        return result;
      }
      change = isOptimizedBetter(existing, minimized.second, id);
      if (change.empty()) {
        return result;
      }
    } else {
      return result;
    }
  }

  // update result
  result.updated = true;
  result.is_new = is_new;

  // write new or optimized program version
  result.program = minimized.second;
  if (Setup::getMiningMode() == MINING_MODE_SERVER) {
    dumpProgram(id, result.program, global_file, submitted_by);
  } else {
    dumpProgram(id, result.program, local_file, submitted_by);
  }

  // send alert
  std::string prefix = is_new ? "First" : change;
  std::string color = is_new ? "good" : "warning";
  alert(result.program, id, prefix, color, submitted_by);

  return result;
}
