#pragma once

#include <string>
#include <vector>

class Git {
 public:
#ifdef _WIN64
  static void ensureEnv(const std::string &key, const std::string &value);

  static void fixWindowsEnv(std::string project_dir = "");
#endif

  static bool git(const std::string &folder, const std::string &args,
                  bool fail_on_error = true);

  static void clone(const std::string &url, const std::string &folder);

  static bool add(const std::string &folder, const std::string &file);

  static bool commit(const std::string &folder, const std::string &message);

  static bool push(const std::string &folder);

  static std::vector<std::pair<std::string, std::string>> status(
      const std::string &folder);

  static std::vector<std::string> log(const std::string &folder,
                                      size_t max_commits);

  static std::vector<std::pair<std::string, std::string>> diffTree(
      const std::string &folder, const std::string &commit_id);

  static void gunzip(const std::string &path, bool keep);
};
