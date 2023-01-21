// Implements thread-safe persistent cache for mapping strings to integers.

#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <string_view>
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
  size_t Add(const std::string &key) {
    // TODO: Use a reader/writer lock.
    std::unique_lock<std::mutex> lock(mutex_);

    const auto iter = by_string_.find(key);
    if (iter != by_string_.end()) {
      return iter->second;
    }

    const size_t rv = by_id_.size();
    by_id_.push_back(key);
    by_string_[key] = rv;
    dirty_.push(rv);
    return rv;
  }

  const std::string &Get(size_t id) const {
    std::unique_lock<std::mutex> lock(mutex_);
    return by_id_.at(id);
  }

  // Returns value if exists, size_t(-1) if not.  Does NOT modify.
  size_t Get(const std::string &key) const {
    // TODO: Use a reader/writer lock.
    std::unique_lock<std::mutex> lock(mutex_);

    const auto iter = by_string_.find(key);
    if (iter != by_string_.end()) {
      return iter->second;
    }

    return static_cast<size_t>(-1);
  }

  // Atomically returns pairs of all dirty items, and clears the dirty flag.
  using DirtyList = std::vector<std::pair<size_t, std::string>>;
  DirtyList GetDirty() {
    DirtyList ret;
    std::unique_lock<std::mutex> lock(mutex_);
    ret.reserve(dirty_.size());
    while (!dirty_.empty()) {
      size_t id = dirty_.front();
      ret.emplace_back(id, by_id_.at(id));
      dirty_.pop();
    }
    return ret;
  }

  size_t size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return by_id_.size();
  }

private:
  // Always sequential, starts at 0.
  std::vector<std::string> by_id_;

  std::unordered_map<std::string, size_t> by_string_;

  // Freshly added items are considered "dirty" until they ar flushe to
  // storage (sqlite table).
  std::queue<size_t> dirty_;

  mutable std::mutex mutex_;
};

// Caches successful lookups in local storage.
// Intended that each thread has its own instance.
class ThreadLocalIdMap {
public:
  ThreadLocalIdMap() = delete;
  ThreadLocalIdMap(IdMap &shared_cache)
      : shared_cache_(shared_cache), by_id_(), by_string_() {
    by_id_.resize(IdMap::kReservedSize);
    by_string_.reserve(IdMap::kReservedSize);
  }

  size_t Add(const std::string &key) {
    const auto iter = by_string_.find(key);
    if (iter != by_string_.end()) {
      return iter->second;
    }

    const size_t id = shared_cache_.Add(key);

    if (id > by_id_.size()) {
      by_id_.resize(id + 1);
    }
    by_id_[id] = key;
    by_string_[key] = id;
    return id;
  }

  const std::string &Get(size_t id) {
    const std::string &foo = by_id_.at(id);
    if (foo.empty()) {
      return shared_cache_.Get(id);
    }
    return foo;
  }

private:
  IdMap &shared_cache_;
  std::vector<std::string> by_id_;
  std::unordered_map<std::string, size_t> by_string_;
};
