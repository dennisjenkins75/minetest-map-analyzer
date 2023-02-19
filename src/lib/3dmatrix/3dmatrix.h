#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "src/lib/map_reader/pos.h"

template <typename TData> class Sparse3DMatrix {
  // Target hash buckets = closets prime to (4*k + 3), where k = expected
  // concurrency, which is 32 threads, each touching 125 adjacent blocks.
  // (4 * 32 * 125 + 3) = 16003.
  // https://primes.utm.edu/lists/small/100000.txt
  static constexpr size_t kHashBucketCount = 16007;

  struct Bucket {
    std::mutex mutex_;
    std::unordered_map<MapBlockPos, TData, PosHashFunc> data_;
    char padding[128 - 40 - 56];
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

private:
  std::vector<Bucket> buckets_;

  template <typename T, std::size_t expected_size,
            std::size_t actual_size = sizeof(T)>
  void check_size() {
    static_assert(expected_size == actual_size, "Size incorrect");
  }
};
