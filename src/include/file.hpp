#pragma once

#include <string>

class Http {
 public:
  static void get(const std::string &url, const std::string &local_path);
};

bool getEnvFlag(const std::string &var);

int64_t getEnvInt(const std::string &var, int64_t default_value);

bool isDir(const std::string &path);

void ensureDir(const std::string &path);

int64_t getFileAgeInDays(const std::string &path);

size_t getMemUsage();

class FolderLock {
 public:
  FolderLock(std::string folder);

  ~FolderLock();

  void release();

 private:
  std::string lockfile;
  int fd;
};
