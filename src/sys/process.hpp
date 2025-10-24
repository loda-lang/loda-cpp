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

#define PROCESS_ERROR_TIMEOUT 124

#ifdef _WIN64
HANDLE createWindowsProcess(const std::string& command);
#else

#endif

bool isChildProcessAlive(HANDLE pid);

// Run a process with arguments and optional output redirection, kill after
// timeoutSeconds. Optionally run the process from workingDir (chdir).
// Returns exit code, or PROCESS_ERROR_TIMEOUT if killed due to timeout.
int execWithTimeout(const std::vector<std::string>& args, int timeoutSeconds,
                    const std::string& outputFile = "",
                    const std::string& workingDir = "");
