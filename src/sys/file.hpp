#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "sys/jute.h"

#ifdef _WIN64
static constexpr char FILE_SEP = '\\';
#else
static constexpr char FILE_SEP = '/';
#endif

void replaceAll(std::string &str, const std::string &from,
                const std::string &to);

bool isFile(const std::string &path);

bool isDir(const std::string &path);

void ensureDir(const std::string &path);

void rmDirRecursive(const std::string &path);

void moveFile(const std::string &from, const std::string &to);

void moveDirToParent(const std::string &path, const std::string &dir,
                     const std::string &new_parent);

bool execCmd(const std::string &cmd, bool fail_on_error = true);

void makeExecutable(const std::string &path);

void ensureTrailingFileSep(std::string &dir);

std::string getHomeDir();

std::string getTmpDir();

void setTmpDir(const std::string &tmp);

std::string getBashRc();

std::string getNullRedirect();

std::string getFileAsString(const std::string &filename,
                            bool fail_on_error = true);

int64_t getFileAgeInDays(const std::string &path);

size_t getMemUsage();

size_t getTotalSystemMem();

std::map<std::string, std::string> readXML(const std::string &path);

int64_t getJInt(jute::jValue &v, const std::string &key, int64_t def);

double getJDouble(jute::jValue &v, const std::string &key, double def);

bool getJBool(jute::jValue &v, const std::string &key, bool def);

class FolderLock {
 public:
  explicit FolderLock(std::string folder);

  ~FolderLock();

  void release();

 private:
  std::string lockfile;
#ifdef _WIN64
  void *fd;
#else
  int fd;
#endif
};
