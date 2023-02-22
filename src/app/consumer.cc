#include <spdlog/spdlog.h>
#include <unordered_set>

#include "src/app/app.h"
#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"
#include "src/lib/map_reader/utils.h"

using PreserveSet = std::unordered_set<MapBlockPos, MapBlockPosHashFunc>;

void UpdatePreserveSet(PreserveSet &preserve_set,
                       const MapBlockPos &mapblock_pos, int radius) {
  for (int z = mapblock_pos.z - radius; z <= mapblock_pos.z + radius; ++z) {
    for (int y = mapblock_pos.y - radius; y <= mapblock_pos.y + radius; ++y) {
      for (int x = mapblock_pos.x - radius; x <= mapblock_pos.x + radius; ++x) {
        preserve_set.insert(MapBlockPos(x, y, z));
      }
    }
  }
}

void App::RunConsumer() {
  spdlog::trace("Consumer entry");
  std::unique_ptr<MapInterface> map =
      MapInterface::Create(config_.driver_type, config_.map_filename);

  ThreadLocalIdMap node_id_cache(node_ids_);
  ThreadLocalIdMap actor_id_cache(actor_ids_);
  PreserveSet preserve_set;

  while (true) {
    const MapBlockKey key = map_block_queue_.Pop();
    if (key.isTombstone()) {
      spdlog::debug("Tombstone");
      break;
    }

    const MapBlockPos mapblock_pos(key.pos);
    std::optional<MapInterface::Blob> raw_data =
        map->LoadMapBlock(mapblock_pos);
    if (!raw_data) {
      spdlog::error("Failed to load mapblock {0} {1}", mapblock_pos.str(),
                    key.pos);
      stats_.bad_map_blocks++;
      continue;
    }

    BlobReader blob(raw_data.value());
    MapBlock mb;

    try {
      mb.deserialize(blob, key.pos, node_id_cache);
    } catch (const SerializationError &err) {
      // TODO: Log these failed blocks and error message to an output table.
      stats_.bad_map_blocks++;
      spdlog::error("Failed to deserialize mapblock {0} {1}. {2}",
                    mapblock_pos.str(), key.pos, err.what());
      continue;
    }

    stats_.good_map_blocks++;
    std::vector<std::unique_ptr<DataWriterNode>> node_queue;
    node_queue.reserve(256);

    bool anthropocene = false;
    uint64_t uniform = 0;

    for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
      const Node &node = mb.nodes()[i];
      const IdMapItem<NodeIdMapExtraInfo> &node_info =
          node_id_cache.Get(node.param0());
      const std::string &owner = node.get_owner();

      const uint64_t owner_id = owner.empty() ? 0 : actor_id_cache.Add(owner);
      const uint64_t minegeld = node.inventory().total_minegeld();
      const bool is_bones = (node_info.key == "bones::bones");
      const bool has_inventory = !node.inventory().empty();

      if (minegeld || is_bones || has_inventory || (owner_id > -0)) {
        auto dwn = std::make_unique<DataWriterNode>();
        dwn->pos = NodePos(mapblock_pos, i);
        dwn->owner_id = owner_id;
        dwn->node_id = node.param0();
        dwn->minegeld = minegeld;
        dwn->inventory = node.inventory(); // deep copy.

        node_queue.push_back(std::move(dwn));
      }

      anthropocene |= node_info.extra.anthropocene;
    }

    if (!node_queue.empty()) {
      data_writer_.EnqueueNodes(std::move(node_queue));
    }

    if (mb.unique_content_ids() == 1) {
      uniform = mb.nodes()[0].param0();
    }

    // Must populate the sprase 3d matrix BEFORE enqueueing the block to the
    // map_block_writer.
    MapBlockData &data = block_data_.Ref(mapblock_pos);
    data.uniform = uniform;
    data.anthropocene = anthropocene;

    map_block_writer_.EnqueueMapBlockPos(mapblock_pos);

    if (anthropocene) {
      UpdatePreserveSet(preserve_set, mapblock_pos, config_.preserve_radius);
    }

    if (preserve_set.size() > config_.preserve_threshold) {
      preserve_queue_.Enqueue(std::move(preserve_set));
      preserve_set.clear();
    }
  }

  preserve_queue_.Enqueue(std::move(preserve_set));

  spdlog::trace("Consumer exit");
}
