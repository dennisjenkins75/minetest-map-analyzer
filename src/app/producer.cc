#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/factory.h"
#include "src/lib/database/db-map-interface.h"

void App::RunProducer() {
  std::unique_ptr<MapInterface> map = CreateMapInterface(config_);

  const auto callback = [this](int64_t id, int64_t mtime) -> bool {
    map_block_queue_.Enqueue(std::move(MapBlockKey(id, mtime)));
    return true;
  };

  map->ProduceMapBlocks(config_.min_pos, config_.max_pos, callback);
  spdlog::info("MapBlocks: {0}", map_block_queue_.size());
  map_block_queue_.SetTombstone();
}
