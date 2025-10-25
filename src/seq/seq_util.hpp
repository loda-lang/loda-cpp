#pragma once

#include "base/uid.hpp"
#include "math/sequence.hpp"

class SequenceUtil {
 public:
  static const size_t DEFAULT_SEQ_LENGTH;

  static const size_t EXTENDED_SEQ_LENGTH;

  static const size_t FULL_SEQ_LENGTH;

  static bool isTooBig(const Number& n);

  static std::string getOeisUrl(UID id);

  static std::string getSeqsFolder(char domain);

  // Evaluate generated code using an external tool. The function writes
  // 'evalCode' to 'toolPath', executes the external tool with 'args', waits up
  // to 'timeoutSeconds' for completion and reads the sequence from
  // 'resultPath' into 'result'. If 'workingDir' is non-empty, the external
  // tool is executed inside that directory. Returns true on success.
  static bool evalFormulaWithExternalTool(const std::string& evalCode,
                                          const std::string& toolName,
                                          const std::string& toolPath,
                                          const std::string& resultPath,
                                          const std::vector<std::string>& args,
                                          int timeoutSeconds, Sequence& result,
                                          const std::string& workingDir = "");
};
