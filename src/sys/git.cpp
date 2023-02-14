#include "sys/git.hpp"

#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

#ifdef _WIN64
std::string getPath() {
  auto p = std::getenv("PATH");
  if (p) {
    return std::string(p);
  }
  return "";
}

void putEnv(const std::string &key, const std::string &value) {
  Log::get().warn("Setting environment variable: " + key + "=" + value);
  if (_putenv_s(key.c_str(), value.c_str())) {
    Log::get().error(
        "Internal error: cannot set " + key + " environment variable", true);
  }
}

void Git::ensureEnv(const std::string &key, const std::string &value) {
  auto p = std::getenv(key.c_str());
  if (!p) {
    putEnv(key, value);
  }
}

void Git::fixWindowsEnv(std::string project_dir) {
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
                         gzip_exe, false, false);
        }
      }
    }
  }
}
#endif

void Git::git(const std::string &folder, const std::string &args) {
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

void Git::clone(const std::string &url, const std::string &folder) {
  git("", "clone " + url + " \"" + folder + "\"");
}

std::string getTmpFile() {
  const int64_t id = Random::get().gen() % 1000;
  return getTmpDir() + "git_" + std::to_string(id) + ".txt";
}

std::vector<std::string> Git::log(const std::string &folder,
                                  size_t max_commits) {
  auto tmp_file = getTmpFile();
  git(folder, "log --oneline --format=\"%H\" -n " +
                  std::to_string(max_commits) + " > \"" + tmp_file + "\"");
  // read commits from file
  std::string line;
  std::vector<std::string> commits;
  std::ifstream in(tmp_file);
  while (std::getline(in, line)) {
    if (!line.empty()) {
      commits.push_back(line);
    }
  }
  std::remove(tmp_file.c_str());
  return commits;
}

std::vector<std::pair<std::string, std::string>> Git::diffTree(
    const std::string &folder, const std::string &commit_id) {
  std::vector<std::pair<std::string, std::string>> result;
  auto tmp_file = getTmpFile();
  git(folder, "diff-tree --no-commit-id --name-status -r " + commit_id +
                  " > \"" + tmp_file + "\"");
  // read changed file names from file
  std::ifstream in(tmp_file);
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::pair<std::string, std::string> entry;
    iss >> entry.first;
    iss >> entry.second;
    result.emplace_back(entry);
  }
  std::remove(tmp_file.c_str());
  return result;
}

void Git::gunzip(const std::string &path) {
#ifdef _WIN64
  const std::string gzip_test = "gzip --version " + getNullRedirect();
  if (system(gzip_test.c_str()) != 0) {
    fixWindowsEnv();  // gzip is included in Git for Windows
  }
#endif
  execCmd("gzip -f -d \"" + path + "\"");
}
