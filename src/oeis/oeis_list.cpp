#include "oeis/oeis_list.hpp"

#include <fstream>

#include "oeis/oeis_sequence.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

const std::string OeisList::INVALID_MATCHES_FILE("invalid_matches.txt");

const std::string& OeisList::getListsHome() {
  static std::string lists_home;
  if (lists_home.empty()) {
    // don't remove the trailing /
    lists_home = Setup::getLodaHome() + "lists" + FILE_SEP;
    ensureDir(lists_home);
  }
  return lists_home;
}

void OeisList::loadList(const std::string& path,
                        std::unordered_set<size_t>& list) {
  Log::get().debug("Loading list " + path);
  std::ifstream names(path);
  if (!names.good()) {
    Log::get().warn("Sequence list not found: " + path);
  }
  std::string line, id;
  list.clear();
  while (std::getline(names, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != 'A') {
      Log::get().error("Error parsing OEIS sequence ID: " + line, true);
    }
    id = "";
    for (char ch : line) {
      if (ch == ':' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\n') {
        break;
      }
      id += ch;
    }
    list.insert(OeisSequence(id).id);
  }
  Log::get().debug("Finished loading of list " + path + " with " +
                   std::to_string(list.size()) + " entries");
}

void OeisList::loadMapWithComments(const std::string& path,
                                   std::map<size_t, std::string>& map) {
  Log::get().debug("Loading map " + path);
  std::ifstream names(path);
  if (!names.good()) {
    Log::get().warn("Sequence list not found: " + path);
  }
  std::string line, id, comment;
  map.clear();
  while (std::getline(names, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != 'A') {
      Log::get().error("Error parsing OEIS sequence ID: " + line, true);
    }
    id = "";
    comment = "";
    bool is_comment = false;
    for (char ch : line) {
      if (!is_comment && ch == ':') {
        is_comment = true;
        continue;
      }
      if (is_comment) {
        comment += ch;
      } else {
        id += ch;
      }
    }
    trimString(comment);
    map[OeisSequence(id).id] = comment;
  }
  Log::get().debug("Finished loading of list " + path + " with " +
                   std::to_string(map.size()) + " entries");
}

bool OeisList::loadMap(const std::string& path,
                       std::map<size_t, int64_t>& map) {
  std::ifstream in(path);
  if (in.good()) {
    Log::get().debug("Loading map " + path);
    map.clear();
    addToMap(in, map);
    Log::get().debug("Finished loading of map " + path + " with " +
                     std::to_string(map.size()) + " entries");
    return true;
  } else {
    return false;
  }
}

void OeisList::addToMap(std::istream& in, std::map<size_t, int64_t>& map) {
  std::string line, id, value;
  OeisSequence seq;
  bool is_value;
  int64_t v;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != 'A') {
      Log::get().error("Error parsing OEIS sequence ID: " + line, true);
    }
    id = "";
    value = "";
    is_value = false;
    for (char ch : line) {
      if (ch == ':' || ch == ';' || ch == ',' || ch == ' ' || ch == '\t') {
        is_value = true;
        continue;
      }
      if (is_value) {
        value += ch;
      } else {
        id += ch;
      }
    }
    if (id.empty() || value.empty()) {
      Log::get().error("Error parsing line: " + line, true);
    }
    seq = OeisSequence(id);
    v = std::stoll(value);
    if (map.find(seq.id) == map.end()) {
      map[seq.id] = v;
    } else {
      map[seq.id] += v;
    }
  }
}

void OeisList::mergeMap(const std::string& file_name,
                        std::map<size_t, int64_t>& map) {
  if (file_name.find(FILE_SEP) != std::string::npos) {
    Log::get().error("Invalid file name for merging map: " + file_name, true);
  }
  FolderLock lock(getListsHome());
  std::ifstream in(getListsHome() + file_name);
  if (in.good()) {
    try {
      addToMap(in, map);
    } catch (...) {
      Log::get().warn("Overwriting corrupt data in " + file_name);
    }
    in.close();
  }
  std::ofstream out(getListsHome() + file_name);
  for (auto it : map) {
    out << OeisSequence::idStr(it.first) << ": " << it.second
        << std::endl;  // flush at every line to avoid corrupt data
  }
  out.close();
  map.clear();
}
