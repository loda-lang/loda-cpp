#pragma once

#include <iostream>
#include <map>
#include <number.hpp>
#include <string>
#include <unordered_set>

class OeisList {
 public:
  static const std::string& getListsHome();

  static void loadList(const std::string& path,
                       std::unordered_set<size_t>& list);

  static bool loadMap(const std::string& path, std::map<size_t, int64_t>& map);

  static void addToMap(std::istream& in, std::map<size_t, int64_t>& map);

  static void mergeMap(const std::string& file_name,
                       std::map<size_t, int64_t>& map);

 private:
  static std::string LISTS_HOME;
};
