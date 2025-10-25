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

// Process error codes used to disambiguate failures in execWithTimeout().
// Child-side distinct exit codes (returned from the child process):
//  - PROCESS_ERROR_CHDIR (126): child failed to chdir(workingDir)
//  - PROCESS_ERROR_OPEN_OUTPUT (125): child failed to open the output file
//  - PROCESS_ERROR_EXEC (127): execvp() failed (command not found / not
//  executable)
// Parent-side sentinel (returned by the parent when it kills a timed-out
// child process):
//  - PROCESS_ERROR_TIMEOUT (124): child was terminated due to timeout
#define PROCESS_ERROR_TIMEOUT 124
#define PROCESS_ERROR_OPEN_OUTPUT 125
#define PROCESS_ERROR_CHDIR 126
#define PROCESS_ERROR_EXEC 127

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
