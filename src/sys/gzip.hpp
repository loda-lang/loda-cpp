#pragma once

#include <string>

/**
 * Decompress a gzip file using zlib.
 *
 * @param path Path to the gzip file (should end with .gz)
 * @param keep If true, keep the original gzip file after decompression
 */
void gunzip(const std::string &path, bool keep);
