#include "file.hpp"

#include <stdlib.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "util.hpp"

// must be before <psapi.h>
#ifdef _WIN64
#include <windows.h>
#endif

#ifdef _WIN64
#include <io.h>
#include <psapi.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if __MACH__
#include <mach/mach.h>
#endif

void replaceAll(std::string &str, const std::string &from,
                const std::string &to) {
  if (from.empty()) {
    return;
  }
  size_t start = 0;
  while ((start = str.find(from, start)) != std::string::npos) {
    str.replace(start, from.length(), to);
    start += to.length();
  }
}

bool isFile(const std::string &path) {
  std::ifstream f(path.c_str());
  return f.good();
}

bool isDir(const std::string &path) {
  struct stat st;
  return (stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFDIR));
}

// need to do this here because of name conflict with Log
#ifdef _WIN64
#include <windows.h>
#endif

void ensureDir(const std::string &path) {
  auto index = path.find_last_of(FILE_SEP);
  if (index != std::string::npos) {
    auto dir = path.substr(0, index);
    if (!isDir(dir)) {
#ifdef _WIN64
      auto cmd = "md \"" + dir + "\"";
#else
      auto cmd = "mkdir -p \"" + dir + "\"";
#endif
      if (system(cmd.c_str()) != 0) {
        Log::get().error("Error creating directory " + dir, true);
      }
    }
  } else {
    Log::get().error("Error determining directory for " + path, true);
  }
}

void execCmd(const std::string &cmd) {
  if (system(cmd.c_str()) != 0) {
    Log::get().error("Error executing command: " + cmd, true);
  }
}

void moveFile(const std::string &from, const std::string &to) {
  // Log::get().warn("Moving file/directory: " + from + " -> " + to);
  execCmd("mv \"" + from + "\" \"" + to + "\"");
}

void gunzip(const std::string &path) {
#ifdef _WIN64
  const std::string gzip_test = "gzip --version " + getNullRedirect();
  if (system(gzip_test.c_str()) == 0) {
    execCmd("gzip -d \"" + path + "\"");
  } else {
    std::string program_files = "C:\\Program Files";
    auto p = std::getenv("PROGRAMFILES");
    if (p) {
      program_files = std::string(p);
    }
    // https://stackoverflow.com/questions/9964865/c-system-not-working-when-there-are-spaces-in-two-different-parameters
    execCmd("cmd /S /C \"\"" + program_files +
            "\\Git\\usr\\bin\\gzip.exe\" -f -d \"" + path + "\"\"");
  }
#else
  execCmd("gzip -f -d \"" + path + "\"");
#endif
}

void makeExecutable(const std::string &path) {
#ifndef _WIN64
  execCmd("chmod u+x \"" + path + "\"");
#endif
}

void ensureTrailingSlash(std::string &dir) {
  if (dir.back() != FILE_SEP) {
    dir += FILE_SEP;
  }
}

std::string getHomeDir() {
  static std::string home;
  if (home.empty()) {
#ifdef _WIN64
    auto d = std::getenv("HOMEDRIVE");
    auto p = std::getenv("HOMEPATH");
    if (!d || !p) {
      Log::get().error("Cannot determine home directory!", true);
    }
    home = std::string(d) + std::string(p);
#else
    auto h = std::getenv("HOME");
    if (!h) {
      Log::get().error("Cannot determine home directory!", true);
    }
    home = std::string(h);
#endif
  }
  return home;
}

std::string getTmpDir() {
#ifdef _WIN64
  char tmp[500];
  if (GetTempPathA(sizeof(tmp), tmp)) {
    return std::string(tmp);
  } else {
    Log::get().error("Cannot determine temp directory", true);
    return {};
  }
#else
  return "/tmp/";
#endif
}

std::string getBashRc() {
#ifndef _WIN64
  auto shell = std::getenv("SHELL");
  if (shell) {
    std::string sh(shell);
    std::string bashrc;
    if (sh == "/bin/bash") {
      bashrc = getHomeDir() + FILE_SEP + ".bashrc";
      if (isFile(bashrc)) {
        return bashrc;
      }
      bashrc = getHomeDir() + FILE_SEP + ".bash_profile";
      if (isFile(bashrc)) {
        return bashrc;
      }
    } else if (sh == "/bin/zsh") {
      bashrc = getHomeDir() + FILE_SEP + ".zshenv";
      if (isFile(bashrc)) {
        return bashrc;
      }
    }
  }
#endif
  return std::string();
}

std::string getNullRedirect() {
#ifdef _WIN64
  return "> nul 2>&1";
#else
  return "> /dev/null 2> /dev/null";
#endif
}

std::string getFileAsString(const std::string &filename) {
  std::ifstream in(filename);
  std::string str;
  if (in.good()) {
    std::string tmp;
    while (getline(in, tmp)) {
      str += tmp;
    }
    in.close();
  }
  if (str.empty()) {
    Log::get().error("Error loading " + filename, true);
  }
  return str;
}

int64_t getFileAgeInDays(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    time_t now = time(0);
    return (now - st.st_mtime) / (3600 * 24);
  }
  return -1;
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

bool hasGit() {
  static int64_t git_check = -1;
  if (git_check == -1) {
    auto git_test = std::string("git --version ") + getNullRedirect();
    git_check = system(git_test.c_str());
  }
  return git_check == 0;
}

FolderLock::FolderLock(std::string folder) {
  // obtain lock
  ensureTrailingSlash(folder);
  ensureDir(folder);
  lockfile = folder + "lock";
  fd = 0;
  Log::get().debug("Acquiring lock " + lockfile);
#ifdef _WIN64
  for (size_t i = 0; i < 600; i++) {  // magic number
    fd = CreateFile(lockfile.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fd != INVALID_HANDLE_VALUE) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  if (fd == INVALID_HANDLE_VALUE) {
    Log::get().error("Cannot create " + lockfile, true);
  }
#else
  while (true) {
    fd = open(lockfile.c_str(), O_CREAT, 0644);
    flock(fd, LOCK_EX);
    struct stat st0, st1;
    fstat(fd, &st0);
    stat(lockfile.c_str(), &st1);
    if (st0.st_ino == st1.st_ino) break;
    close(fd);
  }
#endif
  Log::get().debug("Obtained lock " + lockfile);
}

FolderLock::~FolderLock() { release(); }

void FolderLock::release() {
  if (!fd) {
    return;
  }
  Log::get().debug("Releasing lock " + lockfile);
#ifdef _WIN64
  CloseHandle(fd);
  DeleteFile(lockfile.c_str());
  fd = nullptr;
#else
  unlink(lockfile.c_str());
  flock(fd, LOCK_UN);
  fd = 0;
#endif
}
