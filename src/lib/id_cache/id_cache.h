// Implements thread-safe persistent cache for mapping strings to integers.

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "src/lib/database/db-sqlite3.h"

class IdCache {
public:
  IdCache() : by_id_(), by_string_(), mutex_() {
    by_id_.reserve(65536);
    by_string_.reserve(65536);
  }

  ~IdCache() {}

  void Load(SqliteDb &db, const std::string_view &table_name);

  void Save(SqliteDb &db, const std::string_view &table_name);

  // Returns value, adds to cache if needed.
  int Add(const std::string &key) {
    // TODO: Use a reader/writer lock.
    std::unique_lock<std::mutex> lock(mutex_);

    auto iter = by_string_.find(key);
    if (iter != by_string_.end()) {
      return iter->second;
    }

    int rv = by_id_.size();
    by_id_.push_back(key);
    by_string_[key] = rv;
    return rv;
  }

  const std::string &Get(int id) const {
    std::unique_lock<std::mutex> lock(mutex_);
    return by_id_.at(id);
  }

private:
  // Always sequential, starts at 0.
  std::vector<std::string> by_id_;

  std::unordered_map<std::string, int> by_string_;

  mutable std::mutex mutex_;
};
