#include "pari.hpp"

#include <fstream>
#include <sstream>

#include "log.hpp"

Sequence Pari::eval(const Formula& f, int64_t start, int64_t end) {
  const std::string gpPath("pari-loda.gp");
  const std::string gpResult("pari-result.txt");
  std::ofstream gp(gpPath);
  if (!gp) {
    throw std::runtime_error("error generating gp file");
  }
  gp << f.toString(true) << std::endl;
  gp << "for (n = " << start << ", " << end << ", print(a(n)))" << std::endl;
  gp << "quit" << std::endl;
  gp.close();
  std::string cmd = "gp -q " + gpPath + " > " + gpResult;
  if (system(cmd.c_str()) != 0) {
    Log::get().error("Error evaluating PARI code: " + gpPath, true);
  }

  // read result from file
  Sequence seq;
  std::ifstream resultIn(gpResult);
  std::string buf;
  if (!resultIn) {
    Log::get().error("Error reading PARI output", true);
  }
  while (std::getline(resultIn, buf)) {
    seq.push_back(Number(buf));
  }

  // clean up
  std::remove(gpPath.c_str());
  std::remove(gpResult.c_str());

  return seq;
}
