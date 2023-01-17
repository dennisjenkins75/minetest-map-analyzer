#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/factory.h"
#include "src/lib/database/db-map-interface.h"

void App::RunProducer() {
  spdlog::trace("Producer entry");
  std::unique_ptr<MapInterface> map = CreateMapInterface(config_);

  int64_t count = 0;
  auto local_stats = std::make_unique<StatsData>();

  const auto callback = [this, &count, &local_stats](int64_t id,
                                                     int64_t mtime) -> bool {
    map_block_queue_.Enqueue(std::move(MapBlockKey(id, mtime)));
    count++;
    local_stats->queued_map_blocks_++;

    if (local_stats->queued_map_blocks_ >= kStatsBlockFlushLimit) {
      stats_.EnqueueStatsData(std::move(local_stats));
      local_stats = std::make_unique<StatsData>();
    }

    return true;
  };

  map->ProduceMapBlocks(config_.min_pos, config_.max_pos, callback);

  stats_.EnqueueStatsData(std::move(local_stats));

  spdlog::info("Producer enqued {0} mapblocks.", count);
  map_block_queue_.SetTombstone();

  spdlog::trace("Producer exit");
}
