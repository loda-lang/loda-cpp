#include "process.hpp"

#include <stdexcept>

#ifdef _WIN64
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN64

HANDLE createWindowsProcess(const std::string& command) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  LPSTR c = const_cast<LPSTR>(command.c_str());
  if (!CreateProcess(nullptr, c, nullptr, nullptr, false, 0, nullptr, nullptr,
                     &si, &pi)) {
    std::string msg = std::string("Error in CreateProcess: ") +
                      std::to_string(GetLastError());
    throw std::runtime_error(msg);
  }
  return pi.hProcess;
}

#endif

bool isChildProcessAlive(HANDLE pid) {
  if (pid == 0) {
    return false;
  }
#ifdef _WIN64
  DWORD exit_code = STILL_ACTIVE;
  GetExitCodeProcess(pid, &exit_code);
  if (exit_code != STILL_ACTIVE) {
    CloseHandle(pid);
    return false;
  }
  return true;
#else
  return (waitpid(pid, nullptr, WNOHANG) == 0);
#endif
}

size_t getTotalSystemMemory() {
#ifdef _WIN64
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return status.ullTotalPhys;
#else
  auto pages = sysconf(_SC_PHYS_PAGES);
  auto page_size = sysconf(_SC_PAGE_SIZE);
  return pages * page_size;
#endif
}
