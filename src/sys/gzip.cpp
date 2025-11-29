#include "sys/gzip.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <zlib.h>

#include "sys/log.hpp"

static constexpr size_t CHUNK_SIZE = 16384;

void gunzip(const std::string &path, bool keep) {
  // Open the gzip file
  gzFile gz = gzopen(path.c_str(), "rb");
  if (!gz) {
    throw std::runtime_error("Cannot open gzip file: " + path);
  }

  // Determine output path (remove .gz extension)
  std::string out_path = path;
  if (out_path.size() > 3 &&
      out_path.substr(out_path.size() - 3) == ".gz") {
    out_path = out_path.substr(0, out_path.size() - 3);
  } else {
    gzclose(gz);
    throw std::runtime_error("Expected .gz extension: " + path);
  }

  // Open output file
  std::ofstream out(out_path, std::ios::binary);
  if (!out) {
    gzclose(gz);
    throw std::runtime_error("Cannot create output file: " + out_path);
  }

  // Decompress
  std::vector<char> buffer(CHUNK_SIZE);
  int bytes_read;
  while ((bytes_read = gzread(gz, buffer.data(), CHUNK_SIZE)) > 0) {
    out.write(buffer.data(), bytes_read);
    if (!out) {
      gzclose(gz);
      throw std::runtime_error("Error writing to output file: " + out_path);
    }
  }

  // Check for errors
  int err;
  const char *error_msg = gzerror(gz, &err);
  if (err != Z_OK && err != Z_STREAM_END) {
    std::string msg = error_msg ? error_msg : "Unknown error";
    gzclose(gz);
    throw std::runtime_error("Error decompressing file: " + path + " - " + msg);
  }

  gzclose(gz);
  out.close();

  // Remove original file if not keeping
  if (!keep) {
    if (std::remove(path.c_str()) != 0) {
      Log::get().warn("Could not remove original file: " + path);
    }
  }
}
