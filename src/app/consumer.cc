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

void App::RunConsumer() {
  spdlog::trace("Consumer entry");
  std::unique_ptr<MapInterface> map = CreateMapInterface(config_);

  while (true) {
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

    stats_.IncrTotalBlocks();
    stats_.IncrByVersion(mb.version());

    if (!mb.deserialize(blob, key.pos)) {
      stats_.IncrBadBlocks();
      spdlog::error("Failed to deserialize mapblock {0} {1}", pos.str(),
                    key.pos);
      continue;
    }

    for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
      const Node &node = mb.nodes()[i];
      const std::string &name = mb.name_for_id(node.param0());

      stats_.IncrNodeByType(name);

      // TODO: Determine if inventory has anything in it, and if yes,
      // write the node to the output database.

      uint64_t minegeld = node.inventory().total_minegeld();
      if (minegeld > 0) {
        std::cout << "minegeld: " << std::setw(12) << minegeld << " "
                  << NodePos(pos, i).str() << " " << name << "\n";
      }

      if (name == "bones:bones") {
        const std::string owner = node.get_meta("_owner");
        std::cout << "bones: " << NodePos(pos, i).str() << " " << owner << "\n";
      }
    }
  }

  spdlog::trace("Consumer exit");
}
