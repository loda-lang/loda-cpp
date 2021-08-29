#pragma once

#include "generator.hpp"
#include "iterator.hpp"
#include "util.hpp"

class ProgramState
{
public:

  ProgramState();

  void validate() const;

  void load( const std::string& path );

  void save( const std::string& path ) const;

  int64_t index;
  int64_t generated;
  Program start;
  Program current;
  Program end;
};

class GeneratorV4: public Generator
{
public:

  GeneratorV4( const Config &config, const Stats &stats );

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

private:

  void init( const Stats &stats );

  void load();

  std::string getPath( int64_t index ) const;

  std::string home;
  std::string numfiles_path;

  Iterator iterator;
  ProgramState state;
  AdaptiveScheduler scheduler;

};
