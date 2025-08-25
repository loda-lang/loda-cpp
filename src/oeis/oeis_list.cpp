#include "oeis/oeis_list.hpp"

#include <fstream>

#include "lang/program_util.hpp"
#include "seq/managed_sequence.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

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
                        std::unordered_set<UID>& list) {
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
    id = "";
    for (char ch : line) {
      if (ch == ':' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\n') {
        break;
      }
      id += ch;
    }
    list.insert(UID(id));
  }
  Log::get().debug("Finished loading of list " + path + " with " +
                   std::to_string(list.size()) + " entries");
}

bool OeisList::loadMapWithComments(const std::string& path,
                                   std::map<UID, std::string>& map) {
  Log::get().debug("Loading map " + path);
  std::ifstream file(path);
  if (!file.good()) {
    Log::get().warn("Sequence list not found: " + path);
    return false;
  }
  std::string line, id, comment;
  map.clear();
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
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
    map[UID(id)] = comment;
  }
  Log::get().debug("Finished loading of list " + path + " with " +
                   std::to_string(map.size()) + " entries");
  return true;
}

bool OeisList::loadMap(const std::string& path, std::map<UID, int64_t>& map) {
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

void OeisList::addToMap(std::istream& in, std::map<UID, int64_t>& map) {
  std::string line, id, value;
  ManagedSequence seq;
  bool is_value;
  int64_t v;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
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
    auto uid = UID(id);
    v = std::stoll(value);
    if (map.find(uid) == map.end()) {
      map[uid] = v;
    } else {
      map[uid] += v;
    }
  }
}

void OeisList::mergeMap(const std::string& file_name,
                        std::map<UID, int64_t>& map) {
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
    out << it.first.string() << ": " << it.second
        << std::endl;  // flush at every line to avoid corrupt data
  }
  out.close();
  map.clear();
}

void OeisList::saveMapWithComments(const std::string& path,
                                   const std::map<UID, std::string>& map) {
  std::ofstream out(path);
  for (const auto& entry : map) {
    out << entry.first.string() << ": " << entry.second << "\n";
  }
  out.close();
}
