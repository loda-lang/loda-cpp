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

#include "log.hpp"

// must be before <psapi.h>
#ifdef _WIN64
#include <windows.h>
#endif

#ifdef _WIN64
#include <io.h>
#include <psapi.h>
#include <windows.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if __MACH__
#include <mach/mach.h>
#endif

#include "web_client.hpp"

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

std::string getPath() {
  auto p = std::getenv("PATH");
  if (p) {
    return std::string(p);
  }
  return "";
}

void execCmd(const std::string &cmd, bool fail_on_error = true) {
  auto exit_code = system(cmd.c_str());
  if (exit_code != 0) {
    Log::get().error("Error executing command (exit code " +
                         std::to_string(exit_code) + "): " + cmd,
                     fail_on_error);
  }
}

void moveFile(const std::string &from, const std::string &to) {
  // Log::get().warn("Moving file/directory: " + from + " -> " + to);
  execCmd("mv \"" + from + "\" \"" + to + "\"");
}

#ifdef _WIN64
void putEnv(const std::string &key, const std::string &value) {
  Log::get().warn("Setting environment variable: " + key + "=" + value);
  if (_putenv_s(key.c_str(), value.c_str())) {
    Log::get().error(
        "Internal error: cannot set " + key + " environment variable", true);
  }
}

void ensureEnv(const std::string &key, const std::string &value) {
  auto p = std::getenv(key.c_str());
  if (!p) {
    putEnv(key, value);
  }
}

void fixWindowsEnv(std::string project_dir) {
  const std::string sys32 = "C:\\WINDOWS\\system32";
  const std::string ps = "C:\\WINDOWS\\system32\\WindowsPowerShell\\v1.0";
  ensureEnv("COMSPEC", sys32 + FILE_SEP + "cmd.exe");
  ensureEnv("SYSTEMROOT", "C:\\WINDOWS");
  std::string path = getPath();
  if (path.empty()) {
    path = sys32 + ";" + ps;
  }
  std::string program_files = "C:\\Program Files";
  auto p = std::getenv("PROGRAMFILES");
  if (p) {
    program_files = std::string(p);
  }
  ensureTrailingFileSep(program_files);
  if (!project_dir.empty()) {
    ensureTrailingFileSep(project_dir);
  }
  bool update = false;
  if (path.find("Git\\cmd") == std::string::npos) {
    if (!path.empty()) {
      path += ";";
    }
    path += program_files + "Git\\cmd";
    if (!project_dir.empty()) {
      path += ";" + project_dir + "git\\cmd";
    }
    update = true;
  }
  if (path.find("Git\\usr\\bin") == std::string::npos) {
    if (!path.empty()) {
      path += ";";
    }
    path += program_files + "Git\\usr\\bin";
    if (!project_dir.empty()) {
      path += ";" + project_dir + "git\\usr\\bin";
    }
    update = true;
  }
  if (update) {
    // 1) set the path so that we can use the web client!
    putEnv("PATH", path);

    if (!project_dir.empty()) {
      // 2) fetch mingit
      const std::string mingit_zip = project_dir + "mingit.zip";
      const std::string mingit_url =
          "https://github.com/git-for-windows/git/releases/download/"
          "v2.37.1.windows.1/MinGit-2.37.1-64-bit.zip";
      if (!isFile(mingit_zip)) {
        FolderLock lock(project_dir);
        if (!isFile(mingit_zip)) {
          WebClient::get(mingit_url, mingit_zip, false, false);
        }
      }

      // 3) unzip mingit
      const std::string mingit_dir = project_dir + "git";
      const std::string bin_dir = mingit_dir + "\\usr\\bin";
      if (isFile(mingit_zip) && !isDir(bin_dir)) {
        FolderLock lock(project_dir);
        if (!isDir(bin_dir)) {
          ensureDir(mingit_dir);
          execCmd("powershell -command \"Expand-Archive -Force '" + mingit_zip +
                      "' '" + mingit_dir + "'\"",
                  false);
        }
      }

      // 4) fetch gzip.exe
      const std::string gzip_exe = bin_dir + "\\gzip.exe";
      if (isDir(bin_dir) && !isFile(gzip_exe)) {
        FolderLock lock(bin_dir);
        if (!isFile(gzip_exe)) {
          WebClient::get("https://boinc.loda-lang.org/loda/dl/gzip.exe",
                         gzip_exe, false, false)
        }
      }
    }
  }
}
#endif

void gunzip(const std::string &path) {
#ifdef _WIN64
  const std::string gzip_test = "gzip --version " + getNullRedirect();
  if (system(gzip_test.c_str()) != 0) {
    fixWindowsEnv();  // gzip is included in Git for Windows
  }
#endif
  execCmd("gzip -f -d \"" + path + "\"");
}

void git(const std::string &folder, const std::string &args) {
  std::string a;
  if (!folder.empty()) {
    a = "-C \"" + folder;
    if (a[a.length() - 1] == '\\') {
      a = a.substr(0, a.length() - 1);
    }
    a += "\"";
  }
  if (!args.empty()) {
    a += " " + args;
  }
#ifdef _WIN64
  const std::string git_test = "git --version " + getNullRedirect();
  if (system(git_test.c_str()) != 0) {
    fixWindowsEnv();
  }
#endif
  execCmd("git " + a);
}

void makeExecutable(const std::string &path) {
#ifndef _WIN64
  execCmd("chmod u+x \"" + path + "\"");
#endif
}

void ensureTrailingFileSep(std::string &dir) {
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

std::string LODA_TMP_DIR;

std::string getTmpDir() {
  if (LODA_TMP_DIR.empty()) {
#ifdef _WIN64
    char tmp[500];
    if (GetTempPathA(sizeof(tmp), tmp)) {
      LODA_TMP_DIR = std::string(tmp);
    } else {
      Log::get().error("Cannot determine temp directory", true);
      return {};
    }
#else
    LODA_TMP_DIR = "/tmp/";
#endif
  }
  return LODA_TMP_DIR;
}

void setTmpDir(const std::string &tmp) { LODA_TMP_DIR = tmp; }

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

std::string getFileAsString(const std::string &filename, bool fail_on_error) {
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
    Log::get().error("Error loading " + filename, fail_on_error);
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

// TODO: move this to process.hpp
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

std::map<std::string, std::string> readXML(const std::string &path) {
  std::map<std::string, std::string> result;
  std::ifstream in(path);
  std::string line, key, value;
  while (std::getline(in, line)) {
    auto b = line.find('<');
    if (b == std::string::npos) {
      continue;
    }
    line = line.substr(b + 1);
    b = line.find('>');
    if (b == std::string::npos) {
      continue;
    }
    key = line.substr(0, b);
    line = line.substr(b + 1);
    b = line.find("</");
    if (b == std::string::npos) {
      continue;
    }
    value = line.substr(0, b);
    result[key] = value;
    Log::get().debug("read xml tag: " + key + "=" + value);
  }
  return result;
}

int64_t getJInt(jute::jValue &v, const std::string &key, int64_t def) {
  if (v[key].get_type() == jute::jType::JNUMBER) {
    return v[key].as_int();
  }
  return def;
}

double getJDouble(jute::jValue &v, const std::string &key, double def) {
  if (v[key].get_type() == jute::jType::JNUMBER) {
    return v[key].as_double();
  }
  return def;
}

bool getJBool(jute::jValue &v, const std::string &key, bool def) {
  if (v[key].get_type() == jute::jType::JBOOLEAN) {
    return v[key].as_bool();
  }
  return def;
}

FolderLock::FolderLock(std::string folder) {
  // obtain lock
  ensureTrailingFileSep(folder);
  ensureDir(folder);
  lockfile = folder + "lock";
  fd = 0;
  Log::get().debug("Acquiring lock " + lockfile);
#ifdef _WIN64
  for (size_t i = 0; i < 1200; i++) {  // magic number
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
