#include <spdlog/spdlog.h>

#include "src/app/factory.h"
#include "src/app/producer.h"
#include "src/lib/database/db-map-interface.h"

void RunProducer(const Config &config, MapBlockQueue *queue) {
  std::unique_ptr<MapInterface> map = CreateMapInterface(config);

  const auto callback = [queue](int64_t id, int64_t mtime) -> bool {
    queue->Enqueue(std::move(MapBlockKey(id, mtime)));
    return true;
  };

  map->ProduceMapBlocks(config.min_pos, config.max_pos, callback);
  spdlog::info("MapBlocks: {0}", queue->size());
  queue->SetTombstone();
}
