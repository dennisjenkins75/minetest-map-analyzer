#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

template <typename TKey, typename TValue, class KeyHashFunc = std::hash<TKey>>
class HashMap {
private:
  // Value is prime, and determined experimentally (benchmarks).
  static constexpr size_t kHashBucketCount = 1117;

  struct Bucket {
    mutable std::mutex mutex_;
    std::unordered_map<TKey, TValue, KeyHashFunc> data_;
  };

  struct BucketAligned : public Bucket {
    char padding[128 - sizeof(Bucket)];
  };

public:
  HashMap() : buckets_(kHashBucketCount) {
    // Want each 'Bucket' in its own CPU cache line to reduce cache evictions
    // due to adjacent mutex contention.  Values valid for g++ 11.3.1
    check_size<BucketAligned, 128>();
  }
  ~HashMap() {}

  // Return a reference to the indicated position.
  // Returned value is not locked.
  TValue &Ref(const TKey &pos) {
    const size_t h = KeyHashFunc()(pos);
    BucketAligned &bucket = buckets_.at(h % kHashBucketCount);
    std::unique_lock<std::mutex> lock(bucket.mutex_);
    return bucket.data_[pos];
  }

  size_t size() const {
    return std::accumulate(buckets_.cbegin(), buckets_.cend(), 0,
                           [](size_t a, const Bucket &bucket) {
                             std::unique_lock<std::mutex> lock(bucket.mutex_);
                             return a + bucket.data_.size();
                           });
  }

private:
  std::vector<BucketAligned> buckets_;

  template <typename T, std::size_t expected_size,
            std::size_t actual_size = sizeof(T)>
  void check_size() {
    static_assert(expected_size == actual_size, "Size incorrect");
  }
};
