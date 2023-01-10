#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/factory.h"
#include "src/lib/database/db-map-interface.h"

void App::RunProducer() {
  spdlog::trace("Producer entry");
  std::unique_ptr<MapInterface> map = CreateMapInterface(config_);

  int64_t count = 0;
  const auto callback = [this, &count](int64_t id, int64_t mtime) -> bool {
    map_block_queue_.Enqueue(std::move(MapBlockKey(id, mtime)));
    count++;
    return true;
  };

  map->ProduceMapBlocks(config_.min_pos, config_.max_pos, callback);
  spdlog::info("MapBlocks: {0}", count);
  map_block_queue_.SetTombstone();

  spdlog::trace("Producer exit");
}
