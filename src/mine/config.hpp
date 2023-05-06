#pragma once

#include "mine/miner.hpp"
#include "sys/util.hpp"

class ConfigLoader {
 public:
  static Miner::Config load(const Settings& settings);
};
