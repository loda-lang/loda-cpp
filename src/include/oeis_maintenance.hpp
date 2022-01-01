#pragma once

#include "minimizer.hpp"
#include "oeis_manager.hpp"

class OeisMaintenance {
 public:
  OeisMaintenance(const Settings &settings);

  void maintain();

 private:
  size_t checkAndMinimizePrograms();

  Evaluator evaluator;
  Minimizer minimizer;
  OeisManager manager;
};
