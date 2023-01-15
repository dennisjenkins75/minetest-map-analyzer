#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/factory.h"
#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"
#include "src/lib/map_reader/utils.h"

void App::RunConsumer() {
  spdlog::trace("Consumer entry");
  std::unique_ptr<MapInterface> map = CreateMapInterface(config_);

  ThreadLocalIdMap node_id_cache(node_ids_);
  ThreadLocalIdMap actor_id_cache(actor_ids_);

  auto local_stats = std::make_unique<StatsData>();

  while (true) {
    if ((local_stats->good_map_blocks_ + local_stats->bad_map_blocks_) >
        kStatsBlockFlushLimit) {
      stats_.EnqueueStatsData(std::move(local_stats));
      local_stats = std::make_unique<StatsData>();
    }

    const MapBlockKey key = map_block_queue_.Pop();
    if (key.isTombstone()) {
      spdlog::debug("Tombstone");
      break;
    }

    // TODO: rename `pos` to `mapblock_pos` to reduce ambiguity below.
    const MapBlockPos pos(key.pos);
    std::optional<MapInterface::Blob> raw_data = map->LoadMapBlock(pos);
    if (!raw_data) {
      spdlog::error("Failed to load mapblock {0} {1}", pos.str(), key.pos);
      continue;
    }

    BlobReader blob(raw_data.value());
    MapBlock mb;

    try {
      mb.deserialize(blob, key.pos, node_id_cache);
    } catch (const SerializationError &err) {
      // TODO: Log these failed blocks and error message to an output table.
      local_stats->bad_map_blocks_++;
      local_stats->by_version_[mb.version()]++;
      spdlog::error("Failed to deserialize mapblock {0} {1}. {2}", pos.str(),
                    key.pos, err.what());
      continue;
    }

    local_stats->good_map_blocks_++;
    local_stats->by_version_[mb.version()]++;
    std::vector<std::unique_ptr<DataWriterNode>> node_queue;
    node_queue.reserve(256);

    for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
      const Node &node = mb.nodes()[i];
      const std::string &name = node_id_cache.Get(node.param0());
      const std::string &owner = node.get_owner();

      local_stats->by_type_.at(node.param0())++;
      const uint64_t owner_id = owner.empty() ? 0 : actor_id_cache.Add(owner);
      const uint64_t minegeld = node.inventory().total_minegeld();
      const bool is_bones = (name == "bones::bones");

      if (minegeld || is_bones || (owner_id > -0)) {
        auto dwn = std::make_unique<DataWriterNode>();
        dwn->pos = NodePos(pos, i);
        dwn->owner_id = owner_id;
        dwn->node_id = node.param0();
        dwn->minegeld = minegeld;
        dwn->inventory = node.inventory(); // deep copy.

        node_queue.push_back(std::move(dwn));
      }
    }

    if (!node_queue.empty()) {
      data_writer_.EnqueueNodes(std::move(node_queue));
    }

    if (mb.unique_content_ids() == 1) {
      auto block = std::make_unique<DataWriterBlock>();
      block->pos = pos;
      block->uniform = mb.nodes()[0].param0();

      data_writer_.EnqueueBlock(std::move(block));
    }
  }

  stats_.EnqueueStatsData(std::move(local_stats));

  spdlog::trace("Consumer exit");
}
