// Implements thread-safe persistent cache for mapping strings to integers.

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "src/lib/database/db-sqlite3.h"

// Bidirectional string/id mapper meant to be shared between threads.
// However, it is expensive to use, so its better to have each thread
// use the `ThreadLocalIdMap` (below), to cache entries in `IdMap`.
class IdMap {
public:
  static constexpr size_t kReservedSize = 65536;

  IdMap() : by_id_(), by_string_(), mutex_() {
    by_id_.reserve(kReservedSize);
    by_string_.reserve(kReservedSize);
  }

  ~IdMap() {}

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

  size_t size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return by_id_.size();
  }

private:
  // Always sequential, starts at 0.
  std::vector<std::string> by_id_;

  std::unordered_map<std::string, int> by_string_;

  mutable std::mutex mutex_;
};

// Caches successful lookups in local storage.
// Intended that each thread has its own instance.
class ThreadLocalIdMap {
public:
  ThreadLocalIdMap() = delete;
  ThreadLocalIdMap(IdMap &shared_cache)
      : shared_cache_(shared_cache), by_string_() {
    by_string_.reserve(IdMap::kReservedSize);
  }

  int Add(const std::string &key) {
    auto iter = by_string_.find(key);
    if (iter != by_string_.end()) {
      return iter->second;
    }

    int id = shared_cache_.Add(key);
    by_string_[key] = id;
    return id;
  }

private:
  IdMap &shared_cache_;
  std::unordered_map<std::string, int> by_string_;
};
