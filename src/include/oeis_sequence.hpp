#pragma once

#include "sequence.hpp"

class OeisSequence
{
public:

  static const size_t MAX_NUM_TERMS;

  static std::string getHome();

  OeisSequence( size_t id = 0 );

  OeisSequence( std::string id_str );

  OeisSequence( size_t id, const std::string &name, const Sequence &s, const Sequence &full );

  std::string id_str( const std::string &prefix = "A" ) const;

  std::string dir_str() const;

  std::string url_str() const;

  std::string getProgramPath() const;

  std::string getBFilePath() const;

  const Sequence& getFull() const;

  void fetchBFile() const;

  size_t id;
  std::string name;
  Sequence norm;

  friend std::ostream& operator<<( std::ostream &out, const OeisSequence &s );

  std::string to_string() const;

private:

  mutable Sequence full;
  mutable bool attempted_bfile;
  mutable bool loaded_bfile;

};
