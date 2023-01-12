#include <iomanip>
#include <iostream>

#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/factory.h"
#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"
#include "src/lib/map_reader/utils.h"

// Flush stats every this many blocks.
static constexpr size_t kStatsBlockFlushLimit = 256;

void App::RunConsumer() {
  spdlog::trace("Consumer entry");
  std::unique_ptr<MapInterface> map = CreateMapInterface(config_);

  ThreadLocalIdMap node_id_cache(node_ids_);
  ThreadLocalIdMap actor_id_cache(actor_ids_);

  auto local_stats = std::make_unique<StatsData>();

  while (true) {
    if (local_stats->total_map_blocks_ > kStatsBlockFlushLimit) {
      stats_.EnqueueStatsData(std::move(local_stats));
      local_stats = std::make_unique<StatsData>();
    }

    const MapBlockKey key = map_block_queue_.Pop();
    if (key.isTombstone()) {
      spdlog::debug("Tombstone");
      break;
    }
    const MapBlockPos pos(key.pos);
    std::optional<MapInterface::Blob> raw_data = map->LoadMapBlock(pos);
    if (!raw_data) {
      spdlog::error("Failed to load mapblock {0} {1}", pos.str(), key.pos);
      continue;
    }

    BlobReader blob(raw_data.value());
    MapBlock mb;

    local_stats->total_map_blocks_++;

    if (!mb.deserialize(blob, key.pos, node_id_cache)) {
      local_stats->bad_map_blocks_++;
      spdlog::error("Failed to deserialize mapblock {0} {1}", pos.str(),
                    key.pos);
      continue;
    }

    local_stats->by_version_[mb.version()]++;

    for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
      const Node &node = mb.nodes()[i];
      const std::string &name = node_id_cache.Get(node.param0());
      const std::string &owner = node.get_meta_owner();

      local_stats->by_type_.at(node.param0())++;
      if (!owner.empty()) {
        actor_id_cache.Add(owner);
      }

      // TODO: Determine if inventory has anything in it, and if yes,
      // write the node to the output database.

      uint64_t minegeld = node.inventory().total_minegeld();
      if (minegeld > 0) {
        std::cout << "minegeld: " << std::setw(12) << minegeld << " "
                  << NodePos(pos, i).str() << " " << name << "\n";
      }

      if (name == "bones:bones") {
        std::cout << "bones: " << NodePos(pos, i).str() << " " << owner << "\n";
      }
    }
  }

  stats_.EnqueueStatsData(std::move(local_stats));

  spdlog::trace("Consumer exit");
}
