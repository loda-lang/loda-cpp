#include "file.hpp"

#include <stdlib.h>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "util.hpp"

#ifdef _WIN64
#include <io.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if __MACH__
#include <mach/mach.h>
#endif

void Http::get(const std::string &url, const std::string &local_path) {
  const std::string cmd =
      "wget -nv --no-use-server-timestamps -O " + local_path + " " + url;
  if (system(cmd.c_str()) != 0) {
    Log::get().error("Error fetching " + url, true);
  }
}

bool Http::postFile(const std::string &url, const std::string &file_path,
                    const std::string &auth) {
  std::string cmd = "curl -i -s";
  if (!auth.empty()) {
    cmd += " -u " + auth;
  }
  cmd += " -XPOST '" + url + "' --data-binary @" + file_path + " > /dev/null";
  auto exit_code = system(cmd.c_str());
  return (exit_code == 0);
}

bool getEnvFlag(const std::string &var) {
  auto t = std::getenv(var.c_str());
  if (t) {
    std::string s(t);
    return (s == "yes" || s == "true");
  }
  return false;
}

int64_t getEnvInt(const std::string &var, int64_t default_value) {
  auto t = std::getenv(var.c_str());
  if (t) {
    std::string s(t);
    return std::stoll(s);
  }
  return default_value;
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
  auto index = path.find_last_of("/");
  if (index != std::string::npos) {
    auto dir = path.substr(0, index);
#ifdef _WIN64
    std::replace(dir.begin(), dir.end(), '/', '\\');
    if (!CreateDirectory(dir.c_str(), nullptr) &&
        ERROR_ALREADY_EXISTS != GetLastError())
#else
    auto cmd = "mkdir -p " + dir;
    if (system(cmd.c_str()) != 0)
#endif
    {
      Log::get().error("Error creating directory " + dir, true);
    }
  } else {
    Log::get().error("Error determining directory for " + path, true);
  }
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
  // TODO: this does not return the "private" memory usage which we want
  // mach_msg_type_number_t cnt = MACH_TASK_BASIC_INFO_COUNT;
  // mach_task_basic_info info;
  // if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
  //              reinterpret_cast<task_info_t>(&info), &cnt) == KERN_SUCCESS) {
  //  mem_usage = info.resident_size;
  //}
#endif
  // TODO: memory usage on windows
  return mem_usage;
}

FolderLock::FolderLock(std::string folder) {
  // obtain lock
  if (folder[folder.size() - 1] != '/') {
    folder += '/';
  }
  ensureDir(folder);
  lockfile = folder + "lock";
  fd = 0;
#ifndef _WIN64
  Log::get().debug("Acquiring lock " + lockfile);
  while (true) {
    fd = open(lockfile.c_str(), O_CREAT, 0644);
    flock(fd, LOCK_EX);
    struct stat st0, st1;
    fstat(fd, &st0);
    stat(lockfile.c_str(), &st1);
    if (st0.st_ino == st1.st_ino) break;
    close(fd);
  }
  Log::get().debug("Obtained lock " + lockfile);
#endif
  // TODO: locks on windows
}

FolderLock::~FolderLock() { release(); }

void FolderLock::release() {
#ifndef _WIN64
  if (fd) {
    Log::get().debug("Releasing lock " + lockfile);
    unlink(lockfile.c_str());
    flock(fd, LOCK_UN);
    fd = 0;
  }
#endif
  // TODO: locks on windows
}
