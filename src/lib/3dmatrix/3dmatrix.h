#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

template <typename TData> class Sparse3DMatrix {
  // Mapblock coords are -2048 to +2047 for each axis.
  // Exactly 2^12 (4096) total values.
  // This gives a maximum of 64B possible MapBlocks.
  static constexpr size_t kRankSize = 4096;
  static constexpr int kMinValue = -2048;
  static constexpr int kMaxValue = 2047;

  // We must use smart pointers b/c "std::mutex" is not copy-constructable,
  // which is a requirement of std::vector.

  struct YRank {
    YRank() : mutex_(), data_() {}
    std::mutex mutex_;
    std::unordered_map<int, TData> data_;

    TData &Ref(int y) {
      if ((y < kMinValue) || (y > kMaxValue)) {
        throw std::out_of_range("YRank::Ref");
      }
      y -= kMinValue;

      std::unique_lock<std::mutex> lock(mutex_);
      auto iter = data_.find(y);
      if (iter == data_.end()) {
        iter = data_.emplace(y, TData()).first;
      }
      lock.unlock();
      return iter->second;
    }
  };

  struct XRank {
    XRank() : mutex_(), yrank_() {}
    std::mutex mutex_;
    std::unordered_map<int, std::unique_ptr<YRank>> yrank_;

    YRank &Ref(int x) {
      if ((x < kMinValue) || (x > kMaxValue)) {
        throw std::out_of_range("XRank::Ref");
      }
      x -= kMinValue;

      std::unique_lock<std::mutex> lock(mutex_);
      auto iter = yrank_.find(x);
      if (iter == yrank_.end()) {
        iter = yrank_.emplace(x, std::make_unique<YRank>()).first;
      }
      lock.unlock();
      return *(iter->second.get());
    }
  };

  struct ZRank {
    ZRank() : mutex_(), xrank_() { xrank_.resize(kRankSize); }
    std::mutex mutex_;
    std::vector<std::unique_ptr<XRank>> xrank_;

    XRank &Ref(int z) {
      if ((z < kMinValue) || (z > kMaxValue)) {
        throw std::out_of_range("ZRank::Ref");
      }
      z -= kMinValue;

      std::unique_lock<std::mutex> lock(mutex_);
      if (!xrank_.at(z)) {
        xrank_.at(z) = std::make_unique<XRank>();
      }
      lock.unlock();

      return *(xrank_.at(z).get());
    }
  };

public:
  Sparse3DMatrix() : data_() {}
  ~Sparse3DMatrix() {}

  // Return a reference to the indicated position.
  // Sequentially locks each rank, so that multiple threads can access
  // different ranks simultaneously.  Returned value is not locked.
  TData &Ref(int x, int y, int z) { return data_.Ref(z).Ref(x).Ref(y); }

private:
  // Triple-nested vector of coordinate dimension (z, x, y) to TData
  ZRank data_;
};
