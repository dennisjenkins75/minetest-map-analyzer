#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "src/lib/map_reader/pos.h"

template <typename TData> class Sparse3DMatrix {
  // Value is prime, and determined experimentally (benchmarks).
  static constexpr size_t kHashBucketCount = 1117;

  struct Bucket {
    mutable std::mutex mutex_;
    std::unordered_map<MapBlockPos, TData, MapBlockPosHashFunc> data_;
    char padding[128 - 40 - 56];
  };

  struct BucketStats {
    double load_factor;
    size_t size;
  };

public:
  Sparse3DMatrix() : buckets_(kHashBucketCount) {
    // Want each 'Bucket' in its own CPU cache line to reduce cache evictions
    // due to adjacent mutex contention.  Values valid for g++ 11.3.1
    check_size<std::mutex, 40>();
    check_size<std::unordered_map<int64_t, TData>, 56>();
    check_size<Bucket, 128>();
  }
  ~Sparse3DMatrix() {}

  // Return a reference to the indicated position.
  // Returned value is not locked.
  TData &Ref(const MapBlockPos &pos) {
    const size_t h = PosHashFunc()(pos);
    Bucket &bucket = buckets_.at(h % kHashBucketCount);
    std::unique_lock<std::mutex> lock(bucket.mutex_);
    return bucket.data_[pos];
  }

  std::vector<BucketStats> GetStats() const {
    std::vector<BucketStats> stats;
    std::transform(
        buckets_.cbegin(), buckets_.cend(), std::back_inserter(stats),
        [](const Bucket &bucket) -> BucketStats {
          std::unique_lock<std::mutex> lock(bucket.mutex_);
          return BucketStats{bucket.data_.load_factor(), bucket.data_.size()};
        });
    return stats;
  }

  size_t size() const {
    return std::accumulate(buckets_.cbegin(), buckets_.cend(), 0,
                           [](size_t a, const Bucket &bucket) {
                             std::unique_lock<std::mutex> lock(bucket.mutex_);
                             return a + bucket.data_.size();
                           });
  }

private:
  std::vector<Bucket> buckets_;

  template <typename T, std::size_t expected_size,
            std::size_t actual_size = sizeof(T)>
  void check_size() {
    static_assert(expected_size == actual_size, "Size incorrect");
  }
};
