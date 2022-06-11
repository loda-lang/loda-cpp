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
#include "program_util.hpp"
#include "setup.hpp"
#include "stats.hpp"
#include "util.hpp"
#include "web_client.hpp"

void throwParseError(const std::string &line) {
  Log::get().error("error parsing OEIS line: " + line, true);
}

OeisManager::OeisManager(const Settings &settings,
                         const std::string &stats_home)
    : settings(settings),
      overwrite_mode(ConfigLoader::load(settings).overwrite_mode),
      evaluator(settings),
      finder(settings, evaluator),
      finder_initialized(false),
      optimizer(settings),
      minimizer(settings),
      loaded_count(0),
      total_count(0),
      stats_home(stats_home.empty()
                     ? (Setup::getLodaHome() + "stats" + FILE_SEP)
                     : stats_home) {}

void OeisManager::load() {
  // check if already loaded
  if (total_count > 0) {
    return;
  }

  // first load the custom sequences lists (needs no lock)
  const std::string oeis_dir = Setup::getProgramsHome() + "oeis" + FILE_SEP;
  OeisList::loadList(oeis_dir + "deny.txt", deny_list);
  OeisList::loadList(oeis_dir + "full_check.txt", full_check_list);
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
  const bool prog_exists = (seq.id >= 0) &&
                           (seq.id < stats->all_program_ids.size()) &&
                           stats->all_program_ids[seq.id];

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
          stats->getTransitiveLength(seq.id) > 10;  // magic number
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
      Log::get().info("Creating OEIS index at \"" + Setup::getOeisHome() +
                      "\"");
      ensureDir(Setup::getOeisHome());
    } else {
      Log::get().info("Updating OEIS index (last update " +
                      std::to_string(age_in_days) + " days ago)");
    }
    std::string cmd, path;
    for (auto &file : files) {
      path = Setup::getOeisHome() + file;
      ApiClient::getDefaultInstance().getOeisFile(file, path);
    }
    // update programs repository using git pull
    auto mode = Setup::getMiningMode();
    auto progs_dir = Setup::getProgramsHome();
    if (mode != MINING_MODE_SERVER && isDir(progs_dir + ".git")) {
      Log::get().info("Updating programs repository");
      git(progs_dir, "pull origin main -q --ff-only");
    }
    // clean up local programs folder
    const int64_t max_age = Setup::getMaxLocalProgramAgeInDays();
    const auto local = Setup::getProgramsHome() + "local";
    if (max_age >= 0 && isDir(local) &&
        Setup::getMiningMode() == MiningMode::MINING_MODE_CLIENT) {
      Log::get().info("Cleaning up local programs directory");
      int64_t num_removed = 0;
      for (const auto &it : std::filesystem::directory_iterator(local)) {
        const auto stem = it.path().filename().stem().string();
        const auto ext = it.path().filename().extension().string();
        bool is_program;
        try {
          OeisSequence s(stem);
          is_program = true;
        } catch (const std::exception &) {
          is_program = stem.rfind("api-", 0) == 0;
        }
        is_program = is_program && (ext == ".asm");
        const auto p = it.path().string();
        if (is_program && getFileAgeInDays(p) > max_age) {
          Log::get().debug("Removing \"" + p + "\"");
          std::filesystem::remove(it.path());
          num_removed++;
        }
      }
      if (num_removed > 0) {
        Log::get().info("Removed " + std::to_string(num_removed) +
                        " old local programs");
      }
    }
  }
}

void OeisManager::generateStats(int64_t age_in_days) {
  load();
  std::string msg;
  if (age_in_days < 0) {
    msg = "Generating program stats at \"" + stats_home + "\"";
  } else {
    msg = "Regenerating program stats (last update " +
          std::to_string(age_in_days) + " days ago)";
  }
  Log::get().info(msg);
  stats.reset(new Stats());

  size_t num_processed = 0;
  Program program;
  std::string file_name;
  bool has_b_file, has_program;

  AdaptiveScheduler notify(20);  // magic number
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
      stats->updateProgramStats(s.id, program);
      num_processed++;
    }
    stats->updateSequenceStats(s.id, has_program, has_b_file);
    if (notify.isTargetReached()) {
      notify.reset();
      Log::get().info("Processed " + std::to_string(num_processed) +
                      " programs");
    }
  }

  // write stats
  stats->finalize();
  stats->save(stats_home);

  // done
  Log::get().info("Finished stats generation for " +
                  std::to_string(num_processed) + " programs");
}

void OeisManager::generateLists() {
  load();
  getStats();
  const std::string lists_home = OeisList::getListsHome();
  Log::get().info("Generating program lists at \"" + lists_home + "\"");
  const size_t list_file_size = 50000;
  std::vector<std::stringstream> list_files(1000000 / list_file_size);
  std::stringstream no_loda;
  size_t num_processed = 0;
  std::string buf;
  for (auto &s : sequences) {
    if (s.id == 0) {
      continue;
    }
    if (s.id < stats->all_program_ids.size() && stats->all_program_ids[s.id]) {
      // update program list
      const size_t list_index = (s.id + 1) / list_file_size;
      buf = s.name;
      replaceAll(buf, "{", "\\{");
      replaceAll(buf, "}", "\\}");
      replaceAll(buf, "*", "\\*");
      replaceAll(buf, "_", "\\_");
      replaceAll(buf, "|", "\\|");
      list_files.at(list_index)
          << "* [" << s.id_str() << "](https://oeis.org/" << s.id_str()
          << ") ([program](/edit/?oeis=" << s.id << ")): " << buf << "\n";

      num_processed++;
    } else {
      no_loda << s.id_str() << ": " << s.name << "\n";
    }
  }

  // write lists
  ensureDir(lists_home);
  for (size_t i = 0; i < list_files.size(); i++) {
    auto buf = list_files[i].str();
    if (!buf.empty()) {
      const std::string list_path =
          lists_home + "list" + std::to_string(i) + ".markdown";
      OeisSequence start(std::max<int64_t>(i * list_file_size, 1));
      OeisSequence end(((i + 1) * list_file_size) - 1);
      std::ofstream list_file(list_path);
      list_file << "---\n";
      list_file << "layout: page\n";
      list_file << "title: Programs for " << start.id_str() << "-"
                << end.id_str() << "\n";
      list_file << "permalink: /list" << i << "/\n";
      list_file << "---\n";
      list_file << "List of integer sequences with links to LODA programs."
                << "\n\n";
      list_file << buf;
    }
  }
  std::ofstream no_loda_file(lists_home + "no_loda.txt");
  no_loda_file << no_loda.str();
  no_loda_file.close();

  Log::get().info("Finished generation of lists for " +
                  std::to_string(num_processed) + " programs");
}

void OeisManager::migrate() {
  for (size_t id = 0; id < 400000; id++) {
    OeisSequence s(id);
    std::ifstream f(s.getProgramPath());
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
  if (!stats) {
    // obtain lock
    FolderLock lock(stats_home);

    // create empty stats
    stats.reset(new Stats());

    // check age of stats
    auto age_in_days = getFileAgeInDays(stats->getMainStatsFile(stats_home));
    if (age_in_days < 0 || age_in_days >= Setup::getUpdateIntervalInDays()) {
      generateStats(age_in_days);

      // generate lists in server mode
      if (Setup::getMiningMode() == MINING_MODE_SERVER) {
        generateLists();
      }
    }
    try {
      stats->load(stats_home);
    } catch (const std::exception &e) {
      Log::get().warn("Exception during stats loading, regenerating...");
      generateStats(age_in_days);
      stats->load(stats_home);  // reload
    }
    // lock released at the end of this block
  }

  // publish metrics
  std::vector<Metrics::Entry> entries;
  std::map<std::string, std::string> labels;
  labels["kind"] = "total";
  entries.push_back({"programs", labels, (double)stats->num_programs});
  entries.push_back({"sequences", labels, (double)total_count});
  labels["kind"] = "used";
  entries.push_back({"sequences", labels, (double)stats->num_sequences});
  labels.clear();
  for (size_t i = 0; i < stats->num_ops_per_type.size(); i++) {
    if (stats->num_ops_per_type[i] > 0) {
      labels["type"] =
          Operation::Metadata::get(static_cast<Operation::Type>(i)).name;
      entries.push_back(
          {"operation_types", labels, (double)stats->num_ops_per_type[i]});
    }
  }
  Metrics::get().write(entries);

  return *stats;
}

void OeisManager::releaseStats() { stats.reset(); }

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
    nop.comment = ProgramUtil::PREFIX_SUBMITTED_BY + " " + submitted_by;
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
    buf << ". " << ProgramUtil::PREFIX_SUBMITTED_BY << " " << submitted_by;
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

update_program_result_t OeisManager::updateProgram(
    size_t id, Program p, ValidationMode validation_mode) {
  update_program_result_t result;
  result.updated = false;
  result.is_new = false;
  result.previous_hash = 0;

  // ignore this sequence?
  if (id == 0 || id >= sequences.size() || !sequences[id].id ||
      ignore_list.find(id) != ignore_list.end()) {
    return result;
  }

  auto &seq = sequences[id];
  const std::string global_file = seq.getProgramPath(false);
  const std::string local_file = seq.getProgramPath(true);

  // get metadata from comments
  const std::string submitted_by =
      ProgramUtil::getCommentField(p, ProgramUtil::PREFIX_SUBMITTED_BY);
  const std::string change_type =
      ProgramUtil::getCommentField(p, ProgramUtil::PREFIX_CHANGE_TYPE);
  const std::string previous_hash_str =
      ProgramUtil::getCommentField(p, ProgramUtil::PREFIX_PREVIOUS_HASH);
  size_t previous_hash = 0;
  if (!previous_hash_str.empty()) {
    std::stringstream buf(previous_hash_str);
    buf >> previous_hash;
  }

  // check if there is an existing program already
  const bool has_global = isFile(global_file);
  const bool has_local = isFile(local_file);
  bool is_new = true;
  Program existing;
  if (has_global || has_local) {
    const std::string file_name = has_local ? local_file : global_file;
    try {
      existing = parser.parse(file_name);
      is_new = false;
    } catch (const std::exception &) {
      Log::get().error("Error parsing " + file_name, false);
      existing.ops.clear();
    }
  }

  if (!is_new) {
    // if the programs are exactly the same, no need to evaluate them
    optimizer.removeNops(existing);
    optimizer.removeNops(p);
    if (p == existing) {
      // Log::get().info("Skipping update for " + seq.id_str() + "
      // (unchanged)");
      return result;
    }
  }

  // minimize and check the program
  std::pair<std::string, Program> checked;
  size_t num_default_terms = OeisSequence::EXTENDED_SEQ_LENGTH;
  if (full_check_list.find(seq.id) != full_check_list.end()) {
    num_default_terms = OeisSequence::FULL_SEQ_LENGTH;
  }
  switch (validation_mode) {
    case ValidationMode::BASIC:
      checked = finder.checkProgramBasic(p, existing, is_new, seq, change_type,
                                         previous_hash, num_default_terms);
      break;
    case ValidationMode::EXTENDED:
      checked = finder.checkProgramDefault(p, existing, is_new, seq,
                                           num_default_terms);
      break;
  }
  if (checked.first.empty()) {
    return result;
  }

  // update result
  result.updated = true;
  result.is_new = is_new;
  result.change_type = checked.first;
  if (!is_new) {
    result.previous_hash = ProgramUtil::hash(existing);
  }

  // write new or better program version
  result.program = checked.second;
  if (Setup::getMiningMode() == MINING_MODE_SERVER) {
    dumpProgram(id, result.program, global_file, submitted_by);
  } else {
    dumpProgram(id, result.program, local_file, submitted_by);
  }

  // if not updating, ignore this sequence for future matches;
  // this is important for performance: it is likly that we
  // get many mutations at this point and we want to avoid
  // expensive comparisons with the already found program
  if (is_new && overwrite_mode == OverwriteMode::NONE) {
    auto seq_norm = seq.getTerms(settings.num_terms);
    finder.remove(seq_norm, seq.id);
    ignore_list.insert(seq.id);
  }

  // send alert
  std::string color = is_new ? "good" : "warning";
  alert(result.program, id, checked.first, color, submitted_by);

  return result;
}

// returns false if the program was removed, otherwise true
bool OeisManager::maintainProgram(size_t id) {
  // check if the sequence exists
  if (id >= sequences.size()) {
    return true;
  }
  auto &s = sequences[id];
  if (s.id == 0) {
    return true;
  }

  // try to open the program file
  const std::string file_name = s.getProgramPath();
  std::ifstream program_file(file_name);
  if (!program_file.good()) {
    return true;  // program does not exist
  }

  // check if it is on the deny list
  bool is_okay = (deny_list.find(s.id) == deny_list.end());

  // try to load the program
  Program program;
  std::string submitted_by;
  if (is_okay) {
    Log::get().info("Checking program for " + s.to_string());
    try {
      program = parser.parse(program_file);
      submitted_by = ProgramUtil::getCommentField(
          program, ProgramUtil::PREFIX_SUBMITTED_BY);
    } catch (const std::exception &) {
      is_okay = false;
    }
    program_file.close();
  }

  // get the full number of terms
  auto extended_seq = s.getTerms(OeisSequence::FULL_SEQ_LENGTH);

  // check correctness of the program
  try {
    // don't use incremental evaluation in maintenance to detect bugs
    auto check = evaluator.check(program, extended_seq,
                                 OeisSequence::DEFAULT_SEQ_LENGTH, id, false);
    is_okay = (check.first != status_t::ERROR);  // we allow warnings
  } catch (const std::exception &e) {
    Log::get().error(
        "Error checking " + file_name + ": " + std::string(e.what()), false);
    return true;  // not clear what happened, so don't remove it
  }

  if (!is_okay) {
    // send alert and remove file
    alert(program, id, "Removed invalid", "danger", "");
    remove(file_name.c_str());
    return false;
  } else {
    // minimize and dump the program if it is not protected
    const bool is_protected = (protect_list.find(s.id) != protect_list.end());
    if (!is_protected && !ProgramUtil::isCodedManually(program)) {
      ProgramUtil::removeOps(program, Operation::Type::NOP);
      Program minimized = program;
      minimizer.optimizeAndMinimize(minimized,
                                    OeisSequence::DEFAULT_SEQ_LENGTH);
      dumpProgram(s.id, minimized, file_name, submitted_by);
    }
    return true;
  }
}
