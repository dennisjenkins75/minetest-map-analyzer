#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "src/lib/map_reader/pos.h"

template <typename TData> class Sparse3DMatrix {
  static constexpr size_t kHashBucketCount = 4097;

  struct Bucket {
    std::mutex mutex_;
    std::unordered_map<int64_t, TData> data_;
  };

public:
  Sparse3DMatrix() : buckets_(kHashBucketCount) {}
  ~Sparse3DMatrix() {}

  // Return a reference to the indicated position.
  // Returned value is not locked.
  TData &Ref(const MapBlockPos &pos) {
    const int64_t id = pos.MapBlockId();
    const size_t h = std::hash<int64_t>{}(id) % kHashBucketCount;
    std::unique_lock<std::mutex> lock(buckets_.at(h).mutex_);
    return buckets_.at(h).data_[id];
  }

private:
  // Triple-nested vector of coordinate dimension (z, x, y) to TData
  std::vector<Bucket> buckets_;
};
