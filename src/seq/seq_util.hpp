#pragma once

#include <fstream>
#include <vector>

#include "base/uid.hpp"
#include "math/number.hpp"
#include "math/sequence.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"

class SequenceUtil {
 public:
  static const size_t DEFAULT_SEQ_LENGTH;

  static const size_t EXTENDED_SEQ_LENGTH;

  static const size_t FULL_SEQ_LENGTH;

  static bool isTooBig(const Number& n);

  static std::string getOeisUrl(UID id);

  static std::string getSeqsFolder(char domain);

  // Evaluate code by writing it to a file, executing it with a command,
  // and reading the result from output. Returns true on success.
  static bool evalFormulaWithExternalTool(
      const std::string& evalCode, const std::string& toolName,
      const std::string& toolPath, const std::string& resultPath,
      const std::vector<std::string>& args, int timeoutSeconds,
      Sequence& result) {
    std::ofstream toolFile(toolPath);
    if (!toolFile) {
      Log::get().error("Error generating " + toolName + " file", true);
    }
    toolFile << evalCode;
    toolFile.close();

    int exitCode = execWithTimeout(args, timeoutSeconds, resultPath);
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

    // clean up
    std::remove(toolPath.c_str());
    std::remove(resultPath.c_str());

    return true;
  }
};