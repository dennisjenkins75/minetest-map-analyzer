// Defines a class that holds a list of regexes, used for classifying nodes.

#pragma once

#include <istream>
#include <regex>
#include <string>
#include <vector>

class NameFilter {
public:
  NameFilter() : positive_() {}
  ~NameFilter() {}

  // Parses file, line at a time.
  // Lines beginning with '#' are comments, and are ignored.
  // Blank lines are ignored.
  // Lines beginning with '!' are added to the negative list.
  // Lines beginning with anything else are added to the positive list.
  // Both lists are treated as C++ regexes via "<regex>".
  void Load(std::istream &ifs);

  // Adds a pattern to the list of patterns.
  void Add(const std::string &pattern);

  // Determine if a given string matches one or more regexes in the positive
  // list, and also do NOT match any entries in the negative list. Thread-safe
  // if class is no longer Load()ing data.
  bool Search(const std::string &str) const;

  size_t size() const { return positive_.size() + negative_.size(); }

private:
  std::vector<std::regex> positive_;
  std::vector<std::regex> negative_;
};
