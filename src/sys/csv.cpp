#include "sys/csv.hpp"

#include <sstream>
#include <stdexcept>

CsvReader::CsvReader(const std::string& path) : stream(path), path(path) {
  if (!stream.is_open()) {
    throw std::runtime_error("Failed to open file: " + path);
  }
}

void CsvReader::checkHeader(const std::string& expected_header) {
  std::string line;
  if (!std::getline(stream, line) || line != expected_header) {
    throw std::runtime_error("unexpected header in " + path);
  }
}

bool CsvReader::readRow() {
  std::string line;
  if (!std::getline(stream, line)) {
    return false;
  }

  current_row.clear();
  std::stringstream ss(line);
  std::string field;

  while (std::getline(ss, field, ',')) {
    current_row.push_back(field);
  }

  return true;
}

std::string CsvReader::getField(size_t index) const {
  if (index >= current_row.size()) {
    throw std::runtime_error("Field index out of range");
  }
  return current_row[index];
}

int64_t CsvReader::getIntegerField(size_t index) const {
  return std::stoll(getField(index));
}

size_t CsvReader::numFields() const { return current_row.size(); }

void CsvReader::close() { stream.close(); }

CsvWriter::CsvWriter(const std::string& path)
    : stream(path), separator(",") {
  if (!stream.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }
}

void CsvWriter::writeHeader(const std::string& header) {
  stream << header << "\n";
}

void CsvWriter::writeRow(const std::vector<std::string>& fields) {
  for (size_t i = 0; i < fields.size(); i++) {
    if (i > 0) {
      stream << separator;
    }
    stream << fields[i];
  }
  stream << "\n";
}

void CsvWriter::writeRow(const std::string& field1, const std::string& field2) {
  stream << field1 << separator << field2 << "\n";
}

void CsvWriter::writeRow(const std::string& field1, const std::string& field2,
                         const std::string& field3) {
  stream << field1 << separator << field2 << separator << field3 << "\n";
}

void CsvWriter::close() { stream.close(); }
