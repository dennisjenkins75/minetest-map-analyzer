// Implements thread-safe persistent cache for mapping strings to integers.

#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>

#include "src/lib/database/db-sqlite3.h"

template <typename TExtra> struct IdMapItem {
  IdMapItem() : id(static_cast<size_t>(-1)), key(), extra() {}

  size_t id;
  std::string key;
  TExtra extra;
};

using DirtyList = std::vector<std::pair<size_t, std::string>>;

// Bidirectional string/id mapper meant to be shared between threads.
// However, it is expensive to use, so its better to have each thread
// use the `ThreadLocalIdMap` (below), to cache entries in `IdMap`.
template <typename TExtra> class IdMap {
public:
  static constexpr size_t kReservedSize = 65536;

  IdMap() : mutex_(), by_id_(), by_string_(), callback_(), not_found_item_() {
    by_id_.reserve(kReservedSize);
    by_string_.reserve(kReservedSize);
  }

  explicit IdMap(std::function<TExtra(const std::string &key)> callback)
      : mutex_(), by_id_(), by_string_(), callback_(callback),
        not_found_item_() {
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

    IdMapItem<TExtra> item;
    item.id = by_id_.size();
    item.key = key;
    if (callback_) {
      item.extra = callback_.value()(key);
    }
    by_id_.push_back(item);
    by_string_[key] = item.id;
    dirty_.push(item.id);
    return item.id;
  }

  // Always returns an item.  Throws exception if not found.
  const IdMapItem<TExtra> &Get(size_t id) const {
    std::unique_lock<std::mutex> lock(mutex_);
    return by_id_.at(id);
  }

  // Always returns an item.  `item.id == -1` if key is not found.
  const IdMapItem<TExtra> &Get(const std::string &key) const {
    // TODO: Use a reader/writer lock.
    std::unique_lock<std::mutex> lock(mutex_);

    const auto iter = by_string_.find(key);
    if (iter != by_string_.end()) {
      return by_id_.at(iter->second);
    }

    return not_found_item_;
  }

  // Atomically returns pairs of all dirty items, and clears the dirty flag.
  DirtyList GetDirty() {
    DirtyList ret;
    std::unique_lock<std::mutex> lock(mutex_);
    ret.reserve(dirty_.size());
    while (!dirty_.empty()) {
      size_t id = dirty_.front();
      ret.emplace_back(id, by_id_.at(id).key);
      dirty_.pop();
    }
    return ret;
  }

  size_t size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return by_id_.size();
  }

private:
  mutable std::mutex mutex_;

  // Always sequential, starts at 0.
  std::vector<IdMapItem<TExtra>> by_id_;

  // Maps string to index into `by_id_`.
  std::unordered_map<std::string, size_t> by_string_;

  // Freshly added items are considered "dirty" until they are flushed to
  // storage (sqlite table).
  std::queue<size_t> dirty_;

  // Optional callback for generating TExtra during un-cached Add().
  std::optional<std::function<TExtra(const std::string &key)>> callback_;

  // Value returned from `Get()` if key or id is not found.
  IdMapItem<TExtra> not_found_item_;
};

// Caches successful lookups in local storage.
// Intended that each thread has its own instance.
template <typename TExtra> class ThreadLocalIdMap {
public:
  ThreadLocalIdMap() = delete;
  ThreadLocalIdMap(IdMap<TExtra> &shared_cache)
      : shared_cache_(shared_cache), by_id_(), by_string_() {
    by_id_.resize(IdMap<TExtra>::kReservedSize);
    by_string_.reserve(IdMap<TExtra>::kReservedSize);
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
    by_id_[id] = &(shared_cache_.Get(id));
    by_string_[key] = id;
    return id;
  }

  const IdMapItem<TExtra> &Get(size_t id) const {
    const IdMapItem<TExtra> *foo = by_id_.at(id);
    if (foo->key.empty()) {
      return shared_cache_.Get(id);
    }
    return *foo;
  }

private:
  IdMap<TExtra> &shared_cache_;
  std::vector<const IdMapItem<TExtra> *> by_id_;
  std::unordered_map<std::string, size_t> by_string_;
};
