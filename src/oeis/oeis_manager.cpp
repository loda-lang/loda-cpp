#include "oeis/oeis_manager.hpp"

#include <stdlib.h>
#include <time.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

#include "eval/fold.hpp"
#include "eval/interpreter.hpp"
#include "eval/optimizer.hpp"
#include "form/formula_gen.hpp"
#include "lang/comments.hpp"
#include "lang/program_util.hpp"
#include "mine/config.hpp"
#include "mine/stats.hpp"
#include "oeis/oeis_list.hpp"
#include "oeis/oeis_program.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/log.hpp"
#include "sys/metrics.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

std::string OverrideModeToString(OverwriteMode mode) {
  switch (mode) {
    case OverwriteMode::NONE:
      return "none";
    case OverwriteMode::ALL:
      return "all";
    case OverwriteMode::AUTO:
      return "auto";
  }
  return "unknown";
}

OeisManager::OeisManager(const Settings &settings,
                         const std::string &stats_home)
    : settings(settings),
      overwrite_mode(ConfigLoader::load(settings).overwrite_mode),
      evaluator(settings),
      finder(settings, evaluator),
      finder_initialized(false),
      update_oeis(false),
      update_programs(false),
      optimizer(settings),
      minimizer(settings),
      loader(sequences, settings.num_terms),
      stats_home(stats_home.empty()
                     ? (Setup::getLodaHome() + "stats" + FILE_SEP)
                     : stats_home) {}

void OeisManager::load() {
  // check if already loaded
  if (getTotalCount() > 0) {
    return;
  }

  // first load the custom sequences lists (needs no lock)
  const std::string oeis_dir = Setup::getProgramsHome() + "oeis" + FILE_SEP;
  OeisList::loadList(oeis_dir + "deny.txt", deny_list);
  OeisList::loadList(oeis_dir + "full_check.txt", full_check_list);
  OeisList::loadList(oeis_dir + "overwrite.txt", overwrite_list);
  OeisList::loadList(oeis_dir + "protect.txt", protect_list);

  // load invalid matches map
  invalid_matches.load();

  {
    // obtain lock
    FolderLock lock(Setup::getOeisHome());
    update(false);
    loader.load(Setup::getOeisHome(), 'A');
    // lock released at the end of this block
  }
  loader.checkConsistency();
}

Finder &OeisManager::getFinder() {
  if (!finder_initialized) {
    // generate stats is needed
    getStats();

    const auto config = ConfigLoader::load(settings);
    Log::get().info(
        "Using miner profile \"" + config.name + "\", override: \"" +
        OverrideModeToString(config.overwrite_mode) +
        "\", backoff: " + (config.usesBackoff() ? "true" : "false"));
    ignore_list.clear();
    for (auto &seq : sequences) {
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
    finder.logSummary(loader.getNumLoaded());
  }
  return finder;
}

bool OeisManager::shouldMatch(const ManagedSequence &seq) const {
  if (seq.id.number() == 0) {
    return false;
  }

  // sequence on the deny list?
  if (deny_list.find(seq.id) != deny_list.end()) {
    return false;
  }

  // too many invalid matches already?
  bool too_many_matches = invalid_matches.hasTooMany(seq.id);

  // check if program exists
  const bool prog_exists = stats->all_program_ids.exists(seq.id);

  // program exists and protected?
  if (prog_exists && protect_list.find(seq.id) != protect_list.end()) {
    return false;
  }

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

void OeisManager::update(bool force) {
  std::vector<std::string> files = {"stripped", "names", "offsets"};

  // check whether oeis files need to be updated
  update_oeis = false;
  auto it = files.begin();
  int64_t oeis_age_in_days = -1;
  while (it != files.end()) {
    auto path = Setup::getOeisHome() + *it;
    oeis_age_in_days = getFileAgeInDays(path);
    if (oeis_age_in_days < 0 ||
        oeis_age_in_days >= Setup::getOeisUpdateInterval()) {
      update_oeis = true;
      break;
    }
    it++;
  }

  // check whether programs need to be updated
  update_programs = false;
  const std::string progs_dir = Setup::getProgramsHome();
  const std::string local_dir = progs_dir + "local";
  const std::string update_progs_file = local_dir + FILE_SEP + ".update";
  int64_t programs_age_in_days = getFileAgeInDays(update_progs_file);
  if (programs_age_in_days < 0 ||
      programs_age_in_days >= Setup::getGitHubUpdateInterval()) {
    update_programs = true;
  }

  // force update?
  if (force) {
    update_oeis = true;
    update_programs = true;
  }

  // perform oeis update
  if (update_oeis) {
    if (oeis_age_in_days == -1) {
      Log::get().info("Creating OEIS index at \"" + Setup::getOeisHome() +
                      "\"");
      ensureDir(Setup::getOeisHome());
    } else {
      Log::get().info("Updating OEIS index (last update " +
                      std::to_string(oeis_age_in_days) + " days ago)");
    }
    for (const auto &file : files) {
      const auto path = Setup::getOeisHome() + file;
      ApiClient::getDefaultInstance().getOeisFile(file, path);
    }
  }

  // perform programs update
  if (update_programs) {
    auto mode = Setup::getMiningMode();
    if (mode != MINING_MODE_SERVER && isDir(progs_dir + ".git")) {
      std::string msg("Updating programs repository");
      if (programs_age_in_days >= 0) {
        msg += " (last update " + std::to_string(programs_age_in_days) +
               " days ago)";
      }
      Log::get().info(msg);
      // update programs repository using git pull
      Setup::pullProgramsHome();
    }

    // touch marker file to track the age (even in server mode)
    ensureDir(update_progs_file);
    std::ofstream marker(update_progs_file);
    if (marker) {
      marker << "1" << std::endl;
    } else {
      Log::get().warn("Cannot write update marker: " + update_progs_file);
    }

    // clean up local programs folder
    const int64_t max_age = Setup::getMaxLocalProgramAgeInDays();
    if (max_age >= 0 && isDir(local_dir) &&
        Setup::getMiningMode() == MiningMode::MINING_MODE_CLIENT) {
      Log::get().info("Cleaning up local programs directory");
      int64_t num_removed = 0;
      for (const auto &f : std::filesystem::directory_iterator(local_dir)) {
        const auto stem = f.path().filename().stem().string();
        const auto ext = f.path().filename().extension().string();
        bool is_program;
        try {
          UID s(stem);
          is_program = true;
        } catch (const std::exception &) {
          is_program = stem.rfind("api-", 0) == 0;
        }
        is_program = is_program && (ext == ".asm");
        const auto p = f.path().string();
        if (is_program && getFileAgeInDays(p) > max_age) {
          Log::get().debug("Removing \"" + p + "\"");
          std::filesystem::remove(f.path());
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
  auto start_time = std::chrono::steady_clock::now();
  stats.reset(new Stats());

  size_t num_processed = 0;
  Program program;
  std::string file_name;
  bool has_program, has_formula;

  AdaptiveScheduler notify(20);  // magic number
  for (const auto &s : sequences) {
    file_name = ProgramUtil::getProgramPath(s.id);
    std::ifstream program_file(file_name);
    has_program = false;
    has_formula = false;
    if (program_file.good()) {
      try {
        program = parser.parse(program_file);
        has_program = true;
        has_formula =
            !Comments::getCommentField(program, Comments::PREFIX_FORMULA)
                 .empty();
        ProgramUtil::removeOps(program, Operation::Type::NOP);

        // update stats
        stats->updateProgramStats(s.id, program);
        num_processed++;
      } catch (const std::exception &exc) {
        Log::get().error(
            "Error parsing " + file_name + ": " + std::string(exc.what()),
            false);
      }
    }
    stats->updateSequenceStats(s.id, has_program, has_formula);
    if (notify.isTargetReached()) {
      notify.reset();
      Log::get().info("Processed " + std::to_string(num_processed) +
                      " programs");
    }
  }

  // write stats
  stats->finalize();
  stats->save(stats_home);

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
  auto mem = getMemUsage() / (1024 * 1024);  // convert to MB
  Log::get().info("Generated stats for " + std::to_string(num_processed) +
                  " programs in " + buf.str() +
                  "s; memory usage: " + std::to_string(mem) + " MiB");
}

void OeisManager::generateLists() {
  load();
  getStats();
  const std::string lists_home = OeisList::getListsHome();
  Log::get().debug("Generating program lists at \"" + lists_home + "\"");
  const size_t list_file_size = 50000;
  std::vector<std::stringstream> list_files(1000000 / list_file_size);
  std::stringstream no_loda;
  size_t num_processed = 0;
  std::string buf;
  for (auto &s : sequences) {
    if (s.id.number() == 0 || deny_list.find(s.id) != deny_list.end()) {
      continue;
    }
    if (stats->all_program_ids.exists(s.id)) {
      // update program list
      const size_t list_index = (s.id.number() + 1) / list_file_size;
      buf = s.name;
      replaceAll(buf, "{", "\\{");
      replaceAll(buf, "}", "\\}");
      replaceAll(buf, "*", "\\*");
      replaceAll(buf, "_", "\\_");
      replaceAll(buf, "|", "\\|");
      list_files.at(list_index)
          << "* [" << s.id.string() << "](https://oeis.org/" << s.id.string()
          << ") ([program](/edit/?oeis=" << s.id.number() << ")): " << buf
          << "\n";

      num_processed++;
    } else {
      no_loda << s.id.string() << ": " << s.name << "\n";
    }
  }

  // write lists
  ensureDir(lists_home);
  for (size_t i = 0; i < list_files.size(); i++) {
    const auto f = list_files[i].str();
    if (!f.empty()) {
      const std::string list_path =
          lists_home + "list" + std::to_string(i) + ".markdown";
      UID start('A', std::max<int64_t>(i * list_file_size, 1));
      UID end('A', ((i + 1) * list_file_size) - 1);
      std::ofstream list_file(list_path);
      list_file << "---\n";
      list_file << "layout: page\n";
      list_file << "title: Programs for " << start.string() << "-"
                << end.string() << "\n";
      list_file << "permalink: /list" << i << "/\n";
      list_file << "---\n";
      list_file << "List of integer sequences with links to LODA programs."
                << "\n\n";
      list_file << f;
      list_file << "\n\n[License Info](https://github.com/loda-lang/"
                   "loda-programs#license)\n";
    }
  }
  std::ofstream no_loda_file(lists_home + "no_loda.txt");
  no_loda_file << no_loda.str();
  no_loda_file.close();

  Log::get().info("Generated lists for " + std::to_string(num_processed) +
                  " programs");
}

void OeisManager::migrate() {
  load();
  AdaptiveScheduler scheduler(20);
  for (const auto &s : sequences) {
    const auto path = ProgramUtil::getProgramPath(s.id);
    std::ifstream f(path);
    if (!f.good()) {
      continue;
    }
    Program p;
    p = parser.parse(f);
    f.close();
    const auto submitted_by =
        Comments::getCommentField(p, Comments::PREFIX_SUBMITTED_BY);
    ProgramUtil::removeOps(p, Operation::Type::NOP);
    for (size_t i = 0; i < 3 && i < p.ops.size(); i++) {
      auto &op = p.ops[i];
      if ((op.type == Operation::Type::MOD ||
           op.type == Operation::Type::MIN) &&
          op.source.type == Operand::Type::CONSTANT &&
          op.source.value.asInt() >= 45) {
        p.ops.erase(p.ops.begin() + i);
        auto terms = s.getTerms(100);
        auto result = evaluator.check(p, terms, -1, s.id);
        if (result.first != status_t::ERROR) {
          Log::get().info("Migrating " + s.id.string());
          dumpProgram(s.id, p, path, submitted_by);
        }
        break;
      }
    }
    if (scheduler.isTargetReached()) {
      scheduler.reset();
      Log::get().info("Processed " + std::to_string(s.id.number()) +
                      " programs");
    }
  }
}

const SequenceIndex &OeisManager::getSequences() const { return sequences; }

const Stats &OeisManager::getStats() {
  if (!stats) {
    // obtain lock
    FolderLock lock(stats_home);

    // create empty stats
    stats.reset(new Stats());

    // check age of stats
    auto update_interval = std::min<int64_t>(Setup::getOeisUpdateInterval(),
                                             Setup::getGitHubUpdateInterval());
    auto age_in_days = getFileAgeInDays(stats->getMainStatsFile(stats_home));
    if (update_oeis || update_programs || age_in_days < 0 ||
        age_in_days >= update_interval) {
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
  entries.push_back({"sequences", labels, (double)loader.getNumTotal()});
  entries.push_back({"formulas", labels, (double)stats->num_formulas});
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

void OeisManager::addSeqComments(Program &p) const {
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::SEQ &&
        op.source.type == Operand::Type::CONSTANT) {
      const auto id = UID::castFromInt(op.source.value.asInt());
      if (sequences.exists(id)) {
        op.comment = sequences.get(id).name;
      }
    }
  }
}

int64_t OeisManager::updateProgramOffset(UID id, Program &p) const {
  if (!sequences.exists(id)) {
    return 0;
  }
  return ProgramUtil::setOffset(p, sequences.get(id).offset);
}

void OeisManager::updateDependentOffset(UID id, UID used_id, int64_t delta) {
  const auto path = ProgramUtil::getProgramPath(id);
  Program p;
  try {
    p = parser.parse(path);
  } catch (const std::exception &) {
    return;  // ignore this dependent program
  }
  auto submitted_by =
      Comments::getCommentField(p, Comments::PREFIX_SUBMITTED_BY);
  bool updated = false;
  for (size_t i = 0; i < p.ops.size(); i++) {
    const auto &op = p.ops[i];
    if (op.type == Operation::Type::SEQ &&
        op.source.type == Operand::Type::CONSTANT &&
        op.source.value == Number(used_id.number())) {
      Operation add(Operation::Type::ADD, op.target,
                    Operand(Operand::Type::CONSTANT, delta));
      p.ops.insert(p.ops.begin() + i, add);
      updated = true;
      i++;
    }
  }
  if (updated) {
    optimizer.optimize(p);
    dumpProgram(id, p, path, submitted_by);
  }
}

void OeisManager::updateAllDependentOffset(UID id, int64_t delta) {
  if (delta == 0) {
    return;
  }
  const auto &call_graph = getStats().call_graph;
  for (const auto &entry : call_graph) {
    if (entry.second == id) {
      updateDependentOffset(entry.first, entry.second, delta);
    }
  }
}

void OeisManager::dumpProgram(UID id, Program &p, const std::string &file,
                              const std::string &submitted_by) const {
  ProgramUtil::removeOps(p, Operation::Type::NOP);
  Comments::removeComments(p);
  addSeqComments(p);
  ensureDir(file);
  const auto &seq = sequences.get(id);
  Program tmp;
  Operation nop(Operation::Type::NOP);
  nop.comment = seq.to_string();
  tmp.ops.push_back(nop);
  if (!submitted_by.empty()) {
    nop.comment = Comments::PREFIX_SUBMITTED_BY + " " + submitted_by;
    tmp.ops.push_back(nop);
  }
  static constexpr size_t MAX_PRINT_TERMS = 80;   // magic number
  static constexpr size_t MAX_PRINT_CHARS = 500;  // magic number
  nop.comment = seq.getTerms(MAX_PRINT_TERMS).to_string();
  if (nop.comment.size() > MAX_PRINT_CHARS) {
    nop.comment = nop.comment.substr(0, MAX_PRINT_CHARS);
    auto n = nop.comment.find_last_of(',');
    if (n != std::string::npos) {
      nop.comment = nop.comment.substr(0, n);
    }
  }
  tmp.ops.push_back(nop);
  FormulaGenerator generator;
  Formula formula;
  if (generator.generate(p, id.number(), formula, false)) {
    nop.comment = Comments::PREFIX_FORMULA + " " + formula.toString();
    tmp.ops.push_back(nop);
  }
  nop.comment.clear();
  tmp.ops.push_back(nop);
  p.ops.insert(p.ops.begin(), tmp.ops.begin(), tmp.ops.end());
  std::ofstream out(file);
  ProgramUtil::print(p, out);
  out.close();
}

void OeisManager::alert(Program p, UID id, const std::string &prefix,
                        const std::string &color,
                        const std::string &submitted_by) const {
  const auto &seq = sequences.get(id);
  std::string msg, full;
  msg = prefix + " program for " + seq.to_string();
  full = msg + " Terms: " + seq.getTerms(settings.num_terms).to_string();
  FormulaGenerator generator;
  Formula formula;
  if (generator.generate(p, id.number(), formula, false)) {
    full += ". " + Comments::PREFIX_FORMULA + " " + formula.toString();
  }
  if (!submitted_by.empty()) {
    std::string sub = Comments::PREFIX_SUBMITTED_BY + " " + submitted_by;
    msg += " " + sub;
    full += ". " + sub;
  }
  Log::AlertDetails details;
  details.title = seq.id.string();
  details.title_link = SequenceUtil::getOeisUrl(seq.id);
  details.color = color;
  std::stringstream buf;
  // TODO: code block markers must be escaped for Slack, but not for Discord
  buf << full << "\\n```\\n";
  ProgramUtil::removeOps(p, Operation::Type::NOP);
  addSeqComments(p);
  ProgramUtil::print(p, buf, "\\n");
  buf << "```";
  details.text = buf.str();
  Log::get().alert(msg, details);
}

Program OeisManager::getExistingProgram(UID id) {
  const std::string global_file = ProgramUtil::getProgramPath(id, false);
  const std::string local_file = ProgramUtil::getProgramPath(id, true);
  const bool has_global = isFile(global_file);
  const bool has_local = isFile(local_file);
  Program existing;
  if (has_global || has_local) {
    const std::string file_name = has_local ? local_file : global_file;
    try {
      existing = parser.parse(file_name);
    } catch (const std::exception &) {
      Log::get().error("Error parsing " + file_name, false);
      existing.ops.clear();
    }
  }
  return existing;
}

update_program_result_t OeisManager::updateProgram(
    UID id, Program p, ValidationMode validation_mode) {
  update_program_result_t result;
  result.updated = false;
  result.is_new = false;
  result.previous_hash = 0;

  // ignore this sequence?
  if (id.number() == 0 || !sequences.exists(id) ||
      ignore_list.find(id) != ignore_list.end()) {
    return result;
  }

  // get metadata from comments
  const std::string submitted_by =
      Comments::getCommentField(p, Comments::PREFIX_SUBMITTED_BY);
  const std::string change_type =
      Comments::getCommentField(p, Comments::PREFIX_CHANGE_TYPE);
  const std::string previous_hash_str =
      Comments::getCommentField(p, Comments::PREFIX_PREVIOUS_HASH);
  size_t previous_hash = 0;
  if (!previous_hash_str.empty()) {
    std::stringstream buf(previous_hash_str);
    buf >> previous_hash;
  }

  // check if there is an existing program already
  auto &seq = sequences.get(id);
  auto existing = getExistingProgram(id);
  bool is_new = existing.ops.empty();

  if (!is_new) {
    // if the programs are exactly the same, no need to evaluate them
    optimizer.removeNops(existing);
    optimizer.removeNops(p);
    if (p == existing) {
      return result;
    }
  }

  // minimize and check the program
  check_result_t checked;
  bool full_check = full_check_list.find(seq.id) != full_check_list.end();
  auto num_usages = stats->getNumUsages(seq.id);
  switch (validation_mode) {
    case ValidationMode::BASIC:
      checked = finder.getChecker().checkProgramBasic(
          p, existing, is_new, seq, change_type, previous_hash, full_check,
          num_usages);
      break;
    case ValidationMode::EXTENDED:
      checked = finder.getChecker().checkProgramExtended(
          p, existing, is_new, seq, full_check, num_usages);
      break;
  }
  // not better or the same after optimization?
  if (checked.status.empty() || (!is_new && checked.program == existing)) {
    return result;
  }

  // update result
  result.updated = true;
  result.is_new = is_new;
  result.program = checked.program;
  result.change_type = checked.status;
  if (!is_new) {
    result.previous_hash = OeisProgram::getTransitiveProgramHash(existing);
  }

  // write new or better program version
  const bool is_server = (Setup::getMiningMode() == MINING_MODE_SERVER);
  const std::string target_file = ProgramUtil::getProgramPath(id, !is_server);
  auto delta = updateProgramOffset(id, result.program);
  optimizer.optimize(result.program);
  dumpProgram(id, result.program, target_file, submitted_by);
  if (is_server) {
    updateAllDependentOffset(id, delta);
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
  alert(result.program, id, checked.status, color, submitted_by);

  return result;
}

// returns false if the program was removed, otherwise true
bool OeisManager::maintainProgram(UID id, bool eval) {
  // check if the sequence exists
  if (id.number() == 0 || !sequences.exists(id)) {
    return true;
  }
  auto &s = sequences.get(id);

  // try to open the program file
  const std::string file_name = ProgramUtil::getProgramPath(s.id);
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
      submitted_by =
          Comments::getCommentField(program, Comments::PREFIX_SUBMITTED_BY);
    } catch (const std::exception &) {
      is_okay = false;
    }
    program_file.close();
  }

  // check if dependent programs are available and there are no recursions
  if (is_okay) {
    try {
      ProgramCache cache;
      cache.collect(s.id);
    } catch (const std::exception &) {
      is_okay = false;
    }
  }

  // check correctness of the program
  if (is_okay && eval) {
    // get the full number of terms
    auto extended_seq = s.getTerms(SequenceUtil::FULL_SEQ_LENGTH);
    auto num_required = OeisProgram::getNumRequiredTerms(program);
    try {
      auto res = evaluator.check(program, extended_seq, num_required, id);
      if (Signals::HALT) {
        return true;  // interrupted evaluation
      }
      is_okay = (res.first != status_t::ERROR);  // we allow warnings
    } catch (const std::exception &e) {
      Log::get().error(
          "Error checking " + file_name + ": " + std::string(e.what()), false);
      return true;  // not clear what happened, so don't remove it
    }
  }

  // unfold, minimize and dump the program if it is not protected
  const bool is_protected = (protect_list.find(s.id) != protect_list.end());
  if (is_okay && !is_protected && !Comments::isCodedManually(program)) {
    // unfold and evaluation could still fail, so catch errors
    try {
      auto updated = program;  // copy program
      auto delta = updateProgramOffset(id, updated);
      ProgramUtil::removeOps(updated, Operation::Type::NOP);
      Fold::autoUnfold(updated);
      if (eval) {
        auto num_minimize = OeisProgram::getNumMinimizationTerms(program);
        minimizer.optimizeAndMinimize(updated, num_minimize);
      } else {
        optimizer.optimize(updated);
      }
      dumpProgram(s.id, updated, file_name, submitted_by);
      updateAllDependentOffset(s.id, delta);
    } catch (const std::exception &e) {
      is_okay = false;
    }
  }

  if (!is_okay) {
    // send alert and remove file
    alert(program, id, "Removed invalid", "danger", "");
    remove(file_name.c_str());
  }

  return is_okay;
}

std::vector<Program> OeisManager::loadAllPrograms() {
  load();
  auto &program_ids = getStats().all_program_ids;
  const auto num_programs = getStats().num_programs;
  std::vector<Program> programs;
  Log::get().info("Loading " + std::to_string(num_programs) + " programs");
  AdaptiveScheduler scheduler(20);
  int64_t loaded = 0;
  for (auto id : program_ids) {
    std::ifstream in(ProgramUtil::getProgramPath(id));
    if (!in) {
      continue;
    }
    try {
      programs.push_back(parser.parse(in));
      loaded++;
    } catch (const std::exception &e) {
      Log::get().warn("Skipping " + id.string() + ": " + e.what());
      continue;
    }
    if (scheduler.isTargetReached() || loaded == num_programs) {
      scheduler.reset();
      Log::get().info("Loaded " + std::to_string(loaded) + "/" +
                      std::to_string(num_programs) + " programs");
    }
  }
  return programs;
}
