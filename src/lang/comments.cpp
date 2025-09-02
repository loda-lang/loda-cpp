#include "lang/comments.hpp"

#include <cctype>

void Comments::addComment(Program &p, const std::string &comment) {
  Operation nop(Operation::Type::NOP);
  nop.comment = comment;
  p.ops.push_back(nop);
}

void Comments::removeComments(Program &p) {
  for (auto &op : p.ops) {
    op.comment.clear();
  }
}

bool Comments::isCodedManually(const Program &p) {
  for (const auto &op : p.ops) {
    if (op.type == Operation::Type::NOP &&
        op.comment.find(PREFIX_CODED_MANUALLY) != std::string::npos) {
      return true;
    }
  }
  return false;
}

std::string Comments::getCommentField(const Program &p,
                                      const std::string &prefix) {
  for (const auto &op : p.ops) {
    if (op.type == Operation::Type::NOP) {
      auto pos = op.comment.find(prefix);
      if (pos != std::string::npos) {
        return op.comment.substr(pos + prefix.size() + 1);
      }
    }
  }
  return std::string();
}

void Comments::removeCommentField(Program &p, const std::string &prefix) {
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (it->type == Operation::Type::NOP &&
        it->comment.find(prefix) != std::string::npos) {
      it = p.ops.erase(it);
    } else {
      it++;
    }
  }
}

std::string Comments::getSequenceIdFromProgram(const Program &p) {
  std::string id_str;
  if (p.ops.empty()) {
    return id_str;  // not found
  }
  const auto &c = p.ops[0].comment;
  if (c.length() > 1 && c[0] == 'A' && std::isdigit(c[1])) {
    id_str = c.substr(0, 2);
    for (size_t i = 2; i < c.length() && std::isdigit(c[i]); i++) {
      id_str += c[i];
    }
  }
  return id_str;
}

std::string Comments::getSubmitter(const Program &p) {
  return getCommentField(p, PREFIX_SUBMITTED_BY);
}

// prefixes without colon
const std::string Comments::PREFIX_SUBMITTED_BY = "Submitted by";
const std::string Comments::PREFIX_CODED_MANUALLY = "Coded manually";

// prefixes with colon
const std::string Comments::PREFIX_FORMULA = "Formula:";
const std::string Comments::PREFIX_MINER_PROFILE = "Miner Profile:";
const std::string Comments::PREFIX_CHANGE_TYPE = "Change Type:";
const std::string Comments::PREFIX_PREVIOUS_HASH = "Previous Hash:";
