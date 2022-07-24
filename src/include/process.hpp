#pragma once

#include <string>

#ifdef _WIN64

#include <windows.h>

HANDLE createWindowsProcess(const std::string& command);

#else

#include <sys/wait.h>
#include <unistd.h>
typedef int64_t HANDLE;

#endif

bool isChildProcessAlive(HANDLE pid);
