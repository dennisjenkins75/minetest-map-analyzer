#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/lib/database/db-map-interface.h"

static constexpr size_t kDefaultProducerBatchSize = 2048;

void App::RunProducer() {
  spdlog::trace("Producer entry");
  std::unique_ptr<MapInterface> map =
      MapInterface::Create(config_.driver_type, config_.map_filename);

  int64_t count = 0;
  std::vector<MapBlockKey> keys;
  keys.reserve(kDefaultProducerBatchSize);

  const auto callback = [this, &count, &keys](const MapBlockPos &pos) -> bool {
    count++;
    stats_.queued_map_blocks++;

    keys.push_back(MapBlockKey(pos.MapBlockId()));
    if (keys.size() >= kDefaultProducerBatchSize) {
      map_block_queue_.Enqueue(std::move(keys));

      keys.clear();
      keys.reserve(kDefaultProducerBatchSize);
    }

    return true;
  };

  map->ProduceMapBlocks(config_.min_pos, config_.max_pos, callback);

  map_block_queue_.Enqueue(std::move(keys));

  spdlog::info("Producer enqued {0} mapblocks.", count);
  map_block_queue_.SetTombstone();

  spdlog::trace("Producer exit");
}
