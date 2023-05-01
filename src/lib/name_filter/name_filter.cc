#include <algorithm>

#include "src/lib/name_filter/name_filter.h"

static constexpr char kWhitespace[] = " \n\r\t\f\v";

void NameFilter::Load(std::istream &ifs) {
  std::string line;
  while (std::getline(ifs, line)) {
    // Skip blank lines and comments.
    if (line.empty() || (line.at(0) == '#')) {
      continue;
    }

    // Truncate line after first blank space ("rtrim").
    size_t end = line.find_last_not_of(kWhitespace);
    if (end != std::string::npos) {
      line = line.substr(0, end + 1);
    }

    if (line.empty()) {
      continue;
    }

    Add(line);
  }
}

void NameFilter::Add(const std::string &pattern) {
  if (pattern.at(0) == '!') {
    negative_.push_back(std::regex(pattern.data() + 1));
  } else {
    positive_.push_back(std::regex(pattern));
  }
}

bool NameFilter::Search(const std::string &str) const {
  bool found = false;

  for (const auto &p : positive_) {
    std::smatch sm;
    if (std::regex_match(str, sm, p, std::regex_constants::match_default)) {
      found = true;
      break;
    }
  }

  if (!found) {
    return false;
  }

  for (const auto &p : negative_) {
    std::smatch sm;
    if (std::regex_match(str, sm, p, std::regex_constants::match_default)) {
      return false;
    }
  }

  return true;
}
