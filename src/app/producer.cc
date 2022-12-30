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

  /*
    Stats stats;
    while (true) {
      const int64_t pos = sqlite3_column_int64(stmt, 0);
      const uint8_t *data =
          static_cast<const uint8_t *>(sqlite3_column_blob(stmt, 1));
      const size_t data_len = sqlite3_column_bytes(stmt, 1);

      // Sadly, we must make a copy... :(
      const std::vector<uint8_t> raw_data(data, data + data_len);
      BlobReader blob(raw_data);

      process_block(pos, blob, &stats);
    }
  */

  map.reset();
}
