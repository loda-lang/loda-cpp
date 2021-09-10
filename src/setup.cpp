#include "setup.hpp"

std::string& Setup::getLodaHome() {
  // don't remove the trailing /
  static std::string home = std::string(std::getenv("HOME")) + "/.loda/";
  return home;
}
