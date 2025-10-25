#include "seq/seq_util.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include "math/big_number.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"
#include "sys/setup.hpp"

const size_t SequenceUtil::DEFAULT_SEQ_LENGTH = 80;  // magic number

const size_t SequenceUtil::EXTENDED_SEQ_LENGTH = 1000;  // magic number

const size_t SequenceUtil::FULL_SEQ_LENGTH = 100000;  // magic number

bool SequenceUtil::isTooBig(const Number& n) {
  if (n == Number::INF) {
    return true;
  }
  if (USE_BIG_NUMBER) {
    return n.getNumUsedWords() >
           static_cast<int64_t>(BigNumber::NUM_WORDS / 4);  // magic number
  } else {
    static const int64_t NUM_INF = (std::numeric_limits<int64_t>::max)();
    return (n.value > (NUM_INF / 1000)) || (n.value < (NUM_INF / -1000));
  }
}

std::string SequenceUtil::getOeisUrl(UID id) {
  return "https://oeis.org/" + id.string();
}

std::string SequenceUtil::getSeqsFolder(char domain) {
  std::string folder;
  switch (domain) {
    case 'A':
      folder = "oeis";
      break;
    case 'U':
      folder = "user";
      break;
    case 'V':
      folder = "virt";
      break;
    default:
      Log::get().error("Unsupported sequence domain: " + std::string(1, domain),
                       true);
  }
  return Setup::getSeqsHome() + folder + FILE_SEP;
}

bool SequenceUtil::evalFormulaWithExternalTool(
    const std::string& evalCode, const std::string& toolName,
    const std::string& toolPath, const std::string& resultPath,
    const std::vector<std::string>& args, int timeoutSeconds, Sequence& result,
    const std::string& workingDir) {
  // write tool file
  std::ofstream toolFile(toolPath);
  if (!toolFile) {
    Log::get().error("Error generating " + toolName + " file", true);
  }
  toolFile << evalCode;
  toolFile.close();

  int exitCode = execWithTimeout(args, timeoutSeconds, resultPath, workingDir);

  if (exitCode != 0) {
    std::remove(toolPath.c_str());
    // Try to read error message from result file before removing it
    std::string errorMsg;
    std::ifstream errorIn(resultPath);
    if (errorIn) {
      std::string line;
      while (std::getline(errorIn, line) && errorMsg.size() < 500) {
        if (!errorMsg.empty()) errorMsg += "; ";
        errorMsg += line;
      }
      errorIn.close();
    }
    std::remove(resultPath.c_str());
    if (exitCode == PROCESS_ERROR_TIMEOUT) {
      return false;  // timeout
    } else {
      std::string fullMsg = "Error evaluating " + toolName +
                            " code: tool exited with code " +
                            std::to_string(exitCode);
      if (!errorMsg.empty()) {
        fullMsg += " (" + errorMsg + ")";
      }
      Log::get().error(fullMsg, true);
    }
  }

  // read result from file
  result.clear();
  std::ifstream resultIn(resultPath);
  std::string buf;
  if (!resultIn) {
    Log::get().error("Error reading " + toolName + " output", true);
  }
  while (std::getline(resultIn, buf)) {
    result.push_back(Number(buf));
  }

  // clean up temporary files
  std::remove(toolPath.c_str());
  std::remove(resultPath.c_str());

  return true;
}
