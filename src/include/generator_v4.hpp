#pragma once

#include "generator.hpp"
#include "iterator.hpp"

class ProgramState
{
public:

  ProgramState( int64_t index );

  std::string getPath() const;

  void load();

  void save() const;

  const int64_t index;
  int64_t generated;
  Program start;
  Program current;
  Program end;
};

class GeneratorV4: public Generator
{
public:

  GeneratorV4( const Config &config, const Stats &stats, int64_t seed );

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

private:

  void init();

  void load();

  Iterator iterator;

};
