#include "sys/process.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN64
#include <DbgHelp.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <ctime>
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

LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers) {
  HANDLE hDumpFile = CreateFile(L"CrashDump.dmp", GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDumpFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pExceptionPointers;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
                      MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);
    std::cout << "unhandled exception; generated crashdump" << std::endl;
  }
  return EXCEPTION_EXECUTE_HANDLER;
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

int execWithTimeout(const std::vector<std::string>& args, int timeoutSeconds,
                    const std::string& outputFile) {
#if defined(_WIN32) || defined(_WIN64)
  throw std::runtime_error(
      "execWithTimeout is only supported on Unix-like systems");
#else
  // Unix implementation
  int pid = fork();
  if (pid < 0) {
    throw std::runtime_error("fork failed");
  }
  if (pid == 0) {
    // Child
    if (!outputFile.empty()) {
      int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0) _exit(127);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
    std::vector<char*> argv;
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    alarm(timeoutSeconds);
    execvp(argv[0], argv.data());
    _exit(127);
  }
  // Parent
  int status = 0;
  time_t start = time(nullptr);
  while (true) {
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == pid) break;
    if (result == -1) {
      throw std::runtime_error("waitpid failed");
    }
    if (difftime(time(nullptr), start) > timeoutSeconds) {
      kill(pid, SIGKILL);
      return PROCESS_ERROR_TIMEOUT;
    }
    usleep(100000);  // Sleep for 100 ms
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return -1;
#endif
}
