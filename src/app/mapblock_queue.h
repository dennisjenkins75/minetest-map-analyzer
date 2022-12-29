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
};

static inline bool operator<(const MapBlockKey &a, const MapBlockKey &b) {
  if (a.mtime == b.mtime) {
    return a.pos < b.pos;
  }
  return a.mtime < b.mtime;
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
