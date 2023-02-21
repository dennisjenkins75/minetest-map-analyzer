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

void PreserveQueue::DoMerge(MapBlockPosSet &&positions) {
  std::unique_lock<std::mutex> lock(final_mutex_);
  //  size_t before = final_queue_.size();

  for (const MapBlockPos &pos : positions) {
    final_queue_.insert(pos);
  }

  //  size_t after = final_queue_.size();

  MapBlockPosSet foo;
  if (final_queue_.size() > config_.preserve_limit) {
    foo = std::move(final_queue_);
    final_queue_.clear();
  }

  lock.unlock();

#if 0
  size_t delta = after - before;
  float ratio = static_cast<float>(delta) / positions.size();

  spdlog::debug("PreserveQueue::DoMerge size:{0} {1:.4f} {2}", positions.size(),
                ratio, after);
#endif

  if (!foo.empty()) {
    //    spdlog::debug("PreserveQueue::DoMerge drain:{0}", foo.size());

    for (const MapBlockPos pos : foo) {
      block_data_.Ref(pos).preserve = true;
    }
    foo.clear();

#if 0
    const auto stats = block_data_.GetStats();
    for (size_t i = 0; i < std::min(size_t(10), stats.size()); i++) {
      spdlog::debug("3dSparseMatrix Stats: {0} {1} {2}", i, stats[i].size,
                    stats[i].load_factor);
    }
#endif
  }
}
