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

namespace Process {

extern const int ERROR_TIMEOUT;

#ifdef _WIN64
HANDLE createWindowsProcess(const std::string& command);
#else

#endif

bool isChildProcessAlive(HANDLE pid);

// Runs a process with arguments and optional output redirection, kills after
// timeoutSeconds. Returns exit code, or ERROR_TIMEOUT if killed due to timeout.
int runWithTimeout(const std::vector<std::string>& args, int timeoutSeconds,
                   const std::string& outputFile = "");

}  // namespace Process
