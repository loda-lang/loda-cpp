#pragma once

#include "sequence.hpp"

class OeisSequence
{
public:

  static const size_t LONG_SEQ_LENGTH;

  static const size_t VERY_LONG_SEQ_LENGTH;

  static std::string getHome();

  OeisSequence( size_t id = 0 );

  OeisSequence( std::string id_str );

  OeisSequence( size_t id, const std::string &name, const Sequence &full );

  std::string id_str( const std::string &prefix = "A" ) const;

  std::string dir_str() const;

  std::string url_str() const;

  std::string getProgramPath() const;

  std::string getBFilePath() const;

  Sequence getTerms( int64_t max_num_terms ) const;

  void fetchBFile( int64_t max_num_terms ) const;

  size_t id;
  std::string name;

  friend std::ostream& operator<<( std::ostream &out, const OeisSequence &s );

  std::string to_string() const;

private:

  mutable Sequence full;
  mutable bool attempted_bfile;
  mutable bool loaded_bfile;

};
