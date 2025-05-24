#pragma once

#include <string>
#include <vector>

#ifdef _WIN64

#include <windows.h>

#else

#include <sys/wait.h>
#include <unistd.h>
typedef int64_t HANDLE;

#endif

class Process {
 public:
  static const int ERROR_TIMEOUT;

#ifdef _WIN64
  static HANDLE createWindowsProcess(const std::string& command);
#else

#endif

  static bool isChildProcessAlive(HANDLE pid);

  // Runs a process with arguments and optional output redirection, kills after
  // timeoutSeconds. Returns exit code, or ERROR_TIMEOUT if killed due to
  // timeout.
  static int runWithTimeout(const std::vector<std::string>& args,
                            int timeoutSeconds,
                            const std::string& outputFile = "");
};
