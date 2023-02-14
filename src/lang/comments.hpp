#pragma once

#include "lang/program.hpp"

class Comments {
 public:
  static void addComment(Program &p, const std::string &comment);

  static void removeComments(Program &p);

  static bool isCodedManually(const Program &p);

  static std::string getCommentField(const Program &p,
                                     const std::string &prefix);

  static void removeCommentField(Program &p, const std::string &prefix);

  static std::string getSequenceIdFromProgram(const Program &p);

  static const std::string PREFIX_SUBMITTED_BY;
  static const std::string PREFIX_CODED_MANUALLY;
  static const std::string PREFIX_FORMULA;
  static const std::string PREFIX_MINER_PROFILE;
  static const std::string PREFIX_CHANGE_TYPE;
  static const std::string PREFIX_PREVIOUS_HASH;
};
