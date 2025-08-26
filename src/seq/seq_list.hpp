#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "base/uid.hpp"

class SequenceList {
 public:
  static const std::string& getListsHome();

  static void loadList(const std::string& path, std::unordered_set<UID>& list);

  static bool loadMapWithComments(const std::string& path,
                                  std::map<UID, std::string>& map);

  static bool loadMap(const std::string& path, std::map<UID, int64_t>& map);

  static void addToMap(std::istream& in, std::map<UID, int64_t>& map);

  static void mergeMap(const std::string& file_name,
                       std::map<UID, int64_t>& map);

  static void saveMapWithComments(const std::string& path,
                                  const std::map<UID, std::string>& map);
};
