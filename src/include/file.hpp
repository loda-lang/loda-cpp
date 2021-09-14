#pragma once

#include <string>

class Http {
 public:
  static void get(const std::string &url, const std::string &local_path);

  static bool postFile(const std::string &url, const std::string &file_path,
                       const std::string &auth);

 private:
  static int64_t WWW_CLIENT;
  static void initWWWClient();
};

bool getEnvFlag(const std::string &var);

int64_t getEnvInt(const std::string &var, int64_t default_value);

bool isFile(const std::string &path);

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
