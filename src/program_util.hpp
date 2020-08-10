#pragma once

#include "number.hpp"
#include "program.hpp"

#include <map>

class ProgramUtil
{
public:

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

  static void removeOps( Program &p, Operation::Type type );

  static size_t numOps( const Program &p, bool withNops );

  static size_t numOps( const Program &p, Operand::Type type );

  static void print( const Program &p, std::ostream &out );

  static void print( const Operation &op, std::ostream &out, int indent = 0 );

  static size_t hash( const Program &p );

  static size_t hash( const Operation &op );

  static size_t hash( const Operand &op );

};
