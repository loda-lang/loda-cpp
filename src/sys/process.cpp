#include "sys/process.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "sys/log.hpp"

#ifdef _WIN64
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

// Helper: log command context (command args, working directory, output file)
static void logProcessContext(const std::vector<std::string>& args,
                              const std::string& workingDir,
                              const std::string& outputFile,
                              const std::string& prefix,
                              bool as_error = false) {
  std::stringstream ss;
  ss << prefix;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i) ss << ' ';
    ss << args[i];
  }
  ss << "', cwd='" << workingDir << "', out='" << outputFile << "'";
  if (as_error) {
    Log::get().error(ss.str());
  } else {
    Log::get().warn(ss.str());
  }
}

int execWithTimeout(const std::vector<std::string>& args, int timeoutSeconds,
                    const std::string& outputFile,
                    const std::string& workingDir) {
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
    // Change working directory if requested
    if (!workingDir.empty()) {
      if (chdir(workingDir.c_str()) != 0) {
        _exit(PROCESS_ERROR_CHDIR);
      }
    }
    if (!outputFile.empty()) {
      int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0) {
        _exit(PROCESS_ERROR_OPEN_OUTPUT);
      }
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
    std::vector<char*> argv;
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    // Do not set an alarm in the child. The parent enforces timeouts and
    // will kill the child if it exceeds the allowed runtime. Setting an
    // alarm() in the child leads to the child being terminated by SIGALRM
    // which makes diagnosis harder and bypasses parent-side cleanup.
    execvp(argv[0], argv.data());
    // execvp only returns on error. Use PROCESS_ERROR_EXEC for exec failures
    // (command not found or not executable) to preserve conventional meaning.
    _exit(PROCESS_ERROR_EXEC);
  }
  // Parent
  int status = 0;
  time_t start = time(nullptr);
  while (true) {
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == pid) break;
    if (result == -1) {
      Log::get().error(std::string("waitpid failed for pid ") +
                       std::to_string(pid));
      return -1;
    }
    if (difftime(time(nullptr), start) > timeoutSeconds) {
      // Log timeout and command context before killing
      logProcessContext(args, workingDir, outputFile,
                        std::string("Timeout after ") +
                            std::to_string(timeoutSeconds) + "s: cmd='",
                        false);
      if (kill(pid, SIGKILL) == 0) {
        return PROCESS_ERROR_TIMEOUT;
      } else {
        Log::get().error(std::string("Failed to kill timed-out child pid ") +
                         std::to_string(pid));
        return -1;
      }
    }
    usleep(100000);  // Sleep for 100 ms
  }

  if (WIFEXITED(status)) {
    const int exit_code = WEXITSTATUS(status);
    if (exit_code != 0) {
      logProcessContext(args, workingDir, outputFile,
                        std::string("Process exited with code ") +
                            std::to_string(exit_code) + ": cmd='",
                        false);
    }
    return exit_code;
  }
  if (WIFSIGNALED(status)) {
    const int sig = WTERMSIG(status);
    logProcessContext(args, workingDir, outputFile,
                      std::string("Process terminated by signal ") +
                          std::to_string(sig) + ": cmd='",
                      false);
    return -sig;
  }
  return -1;
#endif
}
