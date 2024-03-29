#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

struct MapBlockKey {
  int64_t pos;

  MapBlockKey() = delete;
  MapBlockKey(int64_t pos_) : pos(pos_) {}

  static MapBlockKey MakeTombstone() {
    return MapBlockKey{std::numeric_limits<int64_t>::max()};
  }

  bool operator==(const MapBlockKey &a) const { return (a.pos == pos); }

  bool isTombstone() const {
    return pos == std::numeric_limits<int64_t>::max();
  }
};

class MapBlockQueue final {
public:
  MapBlockQueue() : queue_(), mutex_(), cv_() {}

  ~MapBlockQueue() {}

  void Enqueue(MapBlockKey &&item) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(std::move(item));
    cv_.notify_one();
  }

  void Enqueue(std::vector<MapBlockKey> &&keys) {
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &key : keys) {
      queue_.push(key);
    }
    cv_.notify_one();
  }

  // Enqueues a special MapBlockKey that will always sort LAST and represents
  // a "tombstone", so that consumer threads know to stop waiting for items
  // and to exit.
  void SetTombstone() {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(MapBlockKey::MakeTombstone());
    cv_.notify_all();
  }

  // Attempts to pop and return an item from front of queue.
  // Will block if queue is empty.
  // Will return Tomestone value immediately if queue is tombstoned (but leave
  // it in the queue).
  MapBlockKey Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return !queue_.empty(); });

    MapBlockKey retval = queue_.front();

    if (!retval.isTombstone()) {
      queue_.pop();
    }

    // We might have taken the last item and exposed a tombstone.
    cv_.notify_one();
    return retval;
  }

  size_t size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

  bool empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  // Waits until the queue is either tombstoned, or the timeout is reached.
  // Returns 'true' if the timeout is reached AND the queue still has data.
  // Returns 'false' if the queue is tombstoned.
  // These 5 shitty lines took 30 minutes to debug.  You're welcome.
  template <class Rep, class Period>
  bool idle_wait(const std::chrono::duration<Rep, Period> &timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    return !cv_.wait_for(lock, timeout, [this]() {
      return !queue_.empty() && queue_.front().isTombstone();
    });
  }

private:
  std::queue<MapBlockKey> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};
