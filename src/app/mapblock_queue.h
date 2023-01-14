#ifndef __MTMAP_MAPBLOCK_QUEUE_H
#define __MTMAP_MAPBLOCK_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

struct MapBlockKey {
  int64_t pos;
  int64_t mtime;

  static MapBlockKey MakeTombstone() {
    return MapBlockKey{std::numeric_limits<int64_t>::max(),
                       std::numeric_limits<int64_t>::max()};
  }

  bool operator==(const MapBlockKey &a) const {
    return (a.pos == pos) && (a.mtime == mtime);
  }

  bool isTombstone() const {
    return (pos == std::numeric_limits<int64_t>::max()) &&
           (mtime == std::numeric_limits<int64_t>::max());
  }
};

// 1. Want the youngest mtime, and smallest position to sort to the "top".
// 2. Want "tombstone" to always sort last.
static inline bool operator<(const MapBlockKey &a, const MapBlockKey &b) {
#if 0
  if (a.mtime == b.mtime) {
    return a.pos > b.pos;
  }
  return a.mtime > b.mtime;
#else
  return a.pos > b.pos;
#endif
}

class MapBlockQueue final {
public:
  MapBlockQueue() : queue_(), mutex_(), cv_() {}

  ~MapBlockQueue() {}

  void Enqueue(MapBlockKey &&item) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(std::move(item));
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

    MapBlockKey retval = queue_.top();

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

private:
  std::priority_queue<MapBlockKey> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};

#endif // __MTMAP_MAPBLOCK_QUEUE_H
