#pragma once

#include <string>

// must be before <psapi.h>
#ifdef _WIN64
#include <windows.h>
#endif

#ifdef _WIN64
#include <io.h>
#include <psapi.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN64
HANDLE createWindowsProcess(const std::string& command);
#else
typedef int64_t HANDLE;
#endif

bool isChildProcessAlive(HANDLE pid);

size_t getMemUsage();
