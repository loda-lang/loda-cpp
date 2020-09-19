#pragma once

#include "number.hpp"
#include "program.hpp"

#include <map>

class Stats
{
public:

  Stats();

  void load( const std::string &path );

  void save( const std::string &path );

  void updateProgram( const Program &program );

  void updateSequence( size_t id, bool program_found, bool has_b_file );

  int64_t num_programs;
  int64_t num_sequences;
  std::map<number_t, int64_t> num_constants;
  std::vector<int64_t> num_programs_per_length;
  std::vector<int64_t> num_ops_per_type;
  std::vector<bool> found_programs;
  std::vector<bool> cached_b_files;
};
