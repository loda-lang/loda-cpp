#include "miner.hpp"
#include "util.hpp"

class ConfigLoader {
 public:
  static Miner::Config load(const Settings& settings);

  static bool MAINTAINANCE_MODE;

 private:
  static Miner::Config getMaintenanceConfig();
};
