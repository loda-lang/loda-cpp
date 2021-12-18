#pragma once

#include <string>

#ifdef _WIN64
static constexpr char FILE_SEP = '\\';
#else
static constexpr char FILE_SEP = '/';
#endif

#define STD_FILESYSTEM 1
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 8
#undef STD_FILESYSTEM
#endif
#ifdef STD_FILESYSTEM
#include <filesystem>
#endif

bool isFile(const std::string &path);

bool isDir(const std::string &path);

void ensureDir(const std::string &path);

void moveFile(const std::string &from, const std::string &to);

void gunzip(const std::string &path);

void makeExecutable(const std::string &path);

void ensureTrailingSlash(std::string &dir);

std::string getHomeDir();

std::string getTmpDir();

std::string getBashRc();

std::string getNullRedirect();

std::string getFileAsString(const std::string &filename);

int64_t getFileAgeInDays(const std::string &path);

size_t getMemUsage();

bool hasGit();

class FolderLock {
 public:
  FolderLock(std::string folder);

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
