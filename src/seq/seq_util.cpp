#include "seq/seq_util.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include "math/big_number.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"
#include "sys/setup.hpp"

const size_t SequenceUtil::DEFAULT_SEQ_LENGTH = 80;  // magic number

const size_t SequenceUtil::EXTENDED_SEQ_LENGTH = 1000;  // magic number

const size_t SequenceUtil::FULL_SEQ_LENGTH = 100000;  // magic number

bool SequenceUtil::isTooBig(const Number& n) {
  if (n == Number::INF) {
    return true;
  }
  if (USE_BIG_NUMBER) {
    return n.getNumUsedWords() >
           static_cast<int64_t>(BigNumber::NUM_WORDS / 4);  // magic number
  } else {
    static const int64_t NUM_INF = (std::numeric_limits<int64_t>::max)();
    return (n.value > (NUM_INF / 1000)) || (n.value < (NUM_INF / -1000));
  }
}

std::string SequenceUtil::getOeisUrl(UID id) {
  return "https://oeis.org/" + id.string();
}

std::string SequenceUtil::getSeqsFolder(char domain) {
  std::string folder;
  switch (domain) {
    case 'A':
      folder = "oeis";
      break;
    case 'U':
      folder = "user";
      break;
    case 'V':
      folder = "virt";
      break;
    default:
      Log::get().error("Unsupported sequence domain: " + std::string(1, domain),
                       true);
  }
  return Setup::getSeqsHome() + folder + FILE_SEP;
}

// Helper struct to hold parsed output from external tools
struct ParsedToolOutput {
  std::string errorMsg;    // Contains concatenated error lines (with "***")
  Sequence numericValues;  // Parsed numeric results
  bool hasError() const { return !errorMsg.empty(); }
};

// Detect obvious error markers from external tools (PARI: "***", Lean: "error")
static bool looksLikeErrorLine(const std::string& line) {
  std::string lowered = line;
  std::transform(
      lowered.begin(), lowered.end(), lowered.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return (line.find("***") != std::string::npos) ||
         (lowered.find("error") != std::string::npos);
}

// Check if a string starts with a prefix (case-insensitive)
static bool startsWithIgnoreCase(const std::string& str,
                                 const std::string& prefix) {
  if (str.size() < prefix.size()) {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(str[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
}

// Helper function to parse tool output file, extracting both errors and numbers
static ParsedToolOutput parseToolOutput(
    const std::string& resultPath, const std::string& ignoredPrefix = "") {
  ParsedToolOutput result;
  std::ifstream resultFile(resultPath);
  if (!resultFile) {
    return result;  // Return empty result if file cannot be opened
  }

  std::string line;
  bool inError =
      false;  // once set, all following lines are treated as error text
  while (std::getline(resultFile, line)) {
    // Skip empty lines
    if (line.empty()) {
      continue;
    }

    // Calculate firstNonSpace and trimmed once for reuse
    size_t firstNonSpace = line.find_first_not_of(" \t");
    if (firstNonSpace == std::string::npos) {
      continue;  // Skip whitespace-only lines
    }
    std::string trimmed = line.substr(firstNonSpace);

    // Skip lines with the ignored prefix if specified
    if (!ignoredPrefix.empty() &&
        startsWithIgnoreCase(trimmed, ignoredPrefix)) {
      continue;
    }

    bool currentError = inError || looksLikeErrorLine(line);

    // Try to parse as a number only if we are not already in error mode and
    // the current line does not look like an explicit error
    if (!currentError) {
      try {
        result.numericValues.push_back(Number(trimmed));
        continue;  // successfully parsed numeric output
      } catch (...) {
        currentError = true;
        line = "Non-numeric line: " + trimmed;
      }
    }

    // Record error line and switch to error mode for the rest of the file
    if (currentError) {
      if (!result.errorMsg.empty()) {
        result.errorMsg += "\n";
      }
      result.errorMsg += line;
      inError = true;
    }
  }
  resultFile.close();

  return result;
}

bool SequenceUtil::evalFormulaWithExternalTool(
    const std::string& evalCode, const std::string& toolName,
    const std::string& toolPath, const std::string& resultPath,
    const std::vector<std::string>& args, int timeoutSeconds, Sequence& result,
    const std::string& workingDir, const std::string& stdinContent) {
  // write tool file
  std::ofstream toolFile(toolPath);
  if (!toolFile) {
    Log::get().error("Error generating " + toolName + " file", true);
  }
  toolFile << evalCode;
  toolFile.close();

  int exitCode = execWithTimeout(args, timeoutSeconds, resultPath, workingDir,
                                 stdinContent);

  // Determine ignored prefix based on tool name
  // LEAN outputs "info:" messages that should be ignored
  std::string ignoredPrefix = (toolName == "LEAN") ? "info:" : "";

  // Parse the output file for both errors and numeric values
  auto parsed = parseToolOutput(resultPath, ignoredPrefix);

  // Handle timeouts
  if (exitCode == PROCESS_ERROR_TIMEOUT) {
    std::remove(resultPath.c_str());
    return false;
  }

  // Handle non-zero exit codes
  if (exitCode != 0) {
    std::remove(resultPath.c_str());
    std::string fullMsg = "Error evaluating " + toolName +
                          " code: tool exited with code " +
                          std::to_string(exitCode);
    if (parsed.hasError()) {
      fullMsg += ": " + parsed.errorMsg;
    }
    Log::get().error(fullMsg, true);
  }

  // Handle parsing errors (e.g., non-numeric output)
  if (parsed.hasError()) {
    std::remove(resultPath.c_str());
    Log::get().error(
        "Error parsing " + toolName + " output: " + parsed.errorMsg, true);
  }

  // Handle no numeric output
  if (parsed.numericValues.empty()) {
    std::remove(resultPath.c_str());
    Log::get().error("Error parsing " + toolName + " output: no numeric output",
                     true);
  }

  // Clean up temporary files
  std::remove(toolPath.c_str());
  std::remove(resultPath.c_str());

  // Copy results to output parameter
  result = parsed.numericValues;
  return true;
}
