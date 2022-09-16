#include "process.hpp"

#include <stdexcept>

// must be before <psapi.h>
#ifdef _WIN64
#include <windows.h>
#endif

#ifdef _WIN64
#include <io.h>
#include <psapi.h>
#else
#include <unistd.h>
#endif

#ifdef __MACH__
#include <mach/mach.h>
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

size_t getMemUsage() {
  size_t mem_usage = 0;
#if __linux__
  auto fp = fopen("/proc/self/statm", "r");
  if (fp) {
    long rss = 0;
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
      rss = 0;
    }
    fclose(fp);
    mem_usage = (size_t)(rss) * (size_t)(sysconf(_SC_PAGE_SIZE));
  }
#elif __MACH__
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  task_vm_info_data_t info;
  auto kr = task_info(mach_task_self(), TASK_VM_INFO_PURGEABLE,
                      reinterpret_cast<task_info_t>(&info), &count);
  if (kr == KERN_SUCCESS) {
    mem_usage = info.phys_footprint;
  }
#elif _WIN64
  PROCESS_MEMORY_COUNTERS pmc;
  auto result = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
  if (result) {
    mem_usage = pmc.WorkingSetSize;
  }
#endif
  return mem_usage;
}
