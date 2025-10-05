#pragma once

#include <fstream>
#include <string>
#include <vector>

class CsvReader {
 public:
  explicit CsvReader(const std::string& path);

  // Check if the header matches the expected value
  void checkHeader(const std::string& expected_header);

  // Read the next row, returns false if EOF
  bool readRow();

  // Get a field from the current row by index
  std::string getField(size_t index) const;

  // Get an integer field from the current row by index
  int64_t getIntegerField(size_t index) const;

  // Get number of fields in the current row
  size_t numFields() const;

  // Close the file
  void close();

 private:
  std::ifstream stream;
  std::vector<std::string> current_row;
  std::string path;
};

class CsvWriter {
 public:
  explicit CsvWriter(const std::string& path);

  // Write the header row
  void writeHeader(const std::string& header);

  // Write a row with the given fields
  void writeRow(const std::vector<std::string>& fields);

  // Write a row with two fields (convenience method)
  void writeRow(const std::string& field1, const std::string& field2);

  // Write a row with three fields (convenience method)
  void writeRow(const std::string& field1, const std::string& field2,
                const std::string& field3);

  // Close the file
  void close();

 private:
  std::ofstream stream;
  std::string separator;
};
