#include "oeis/invalid_matches.hpp"

#include "oeis/oeis_list.hpp"
#include "sys/log.hpp"

const std::string FILENAME = "invalid_matches.txt";

InvalidMatches::InvalidMatches()
    : scheduler(1800)  // 30 minutes
{}

void InvalidMatches::load() {
  auto path = OeisList::getListsHome() + FILENAME;
  try {
    OeisList::loadMap(path, invalid_matches);
  } catch (const std::exception&) {
    Log::get().warn("Resetting corrupt file " + path);
    invalid_matches.clear();
    deleteFile();
  }
}

bool InvalidMatches::hasTooMany(size_t id) const {
  auto it = invalid_matches.find(id);
  if (it != invalid_matches.end() && it->second > 0) {
    int64_t r = Random::get().gen() % it->second;
    return r >= 100;
  }
  return false;
}

void InvalidMatches::insert(size_t id) {
  invalid_matches[id]++;
  if (scheduler.isTargetReached()) {
    scheduler.reset();
    Log::get().info("Saving invalid matches stats for " +
                    std::to_string(invalid_matches.size()) + " sequences");
    OeisList::mergeMap(FILENAME, invalid_matches);
  }
}

void InvalidMatches::deleteFile() {
  auto path = OeisList::getListsHome() + FILENAME;
  std::remove(path.c_str());
}
