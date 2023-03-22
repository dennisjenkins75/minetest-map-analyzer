#include <spdlog/spdlog.h>

#include "src/app/preserve_queue.h"

void PreserveQueue::MergeThread() {
  spdlog::trace("PreserveQueue::MergeThread() entry");

  std::unique_lock<std::mutex> lock(merge_mutex_);
  while (true) {
    cv_.wait(lock, [this]() { return !merge_queue_.empty(); });

    // Tombstone?  If yes, leave it on the queue and exit.
    if (merge_queue_.front().empty()) {
      break;
    }

    auto packet = std::move(merge_queue_.front());
    merge_queue_.pop();

    lock.unlock();

    DoMerge(std::move(packet));

    lock.lock();
  }

  spdlog::trace("PreserveQueue::MergeThread() exit");
}

void PreserveQueue::DoMerge(PositionList &&positions) {
  std::unique_lock<std::mutex> lock(final_mutex_);

  for (const MapBlockPos &pos : positions) {
    const int r = config_.preserve_radius;
    for (int z = pos.z - r; z <= pos.z + r; ++z) {
      for (int y = pos.y - r; y <= pos.y + r; ++y) {
        for (int x = pos.x - r; x <= pos.x + r; ++x) {
          final_queue_.insert(MapBlockPos(x, y, z));
        }
      }
    }
  }
}
