// Defines a class that holds a list of regexes, used for classifying nodes.

#pragma once

#include <istream>
#include <regex>
#include <string>
#include <vector>

class NameFilter {
public:
  NameFilter() : patterns_() {}
  ~NameFilter() {}

  // Parses file, line at a time.
  // Lines beginning with '#' are comments, and are ignored.
  // Blank lines are ignored.
  // All other lines are treated as C++ regex via "<regex>".
  void Load(std::istream &ifs);

  // Adds a pattern to the list of patterns.
  void Add(const std::string &pattern);

  // Determine if a given string matches one or more regexes in the input
  // list.  Thread-safe if class is no longer Load()ing data.
  bool Search(const std::string &str) const;

private:
  std::vector<std::regex> patterns_;
};
