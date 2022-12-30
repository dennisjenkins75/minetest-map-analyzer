#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>

#include <spdlog/spdlog.h>

#include "src/app/consumer.h"
#include "src/app/factory.h"
#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"
#include "src/lib/map_reader/utils.h"

struct Stats {
  // Total count of map blocks.
  int64_t total_map_blocks;

  // Count of map blocks that failed to parse.
  int64_t bad_map_blocks;

  // Count of map blocks for each version.
  int64_t by_version[256];

  // Count of nodes of each type.
  std::map<std::string, int64_t> by_type;

  Stats() : total_map_blocks(0), bad_map_blocks(0), by_version{}, by_type() {}
};

void dump_stats(const Stats &stats) {
  std::ofstream ofs("stats.out");

  ofs << "bad_map_blocks: " << stats.bad_map_blocks << "\n";
  ofs << "total_map_blocks: " << stats.total_map_blocks << "\n";
  ofs << "bad %: "
      << 100.0 * static_cast<double>(stats.bad_map_blocks) /
             static_cast<double>(stats.total_map_blocks)
      << "\n";

  for (int i = 0; i < 256; i++) {
    if (stats.by_version[i]) {
      ofs << "version: " << i << " = " << stats.by_version[i] << "\n";
    }
  }
  ofs.close();

  ofs.open("nodes-by-type.out");
  for (const auto &n : stats.by_type) {
    ofs << std::setw(12) << n.second << " " << n.first << "\n";
  }
  ofs.close();
}

void find_currency_hoard(const MapBlockPos &pos, const MapBlock &mb) {
  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];

    uint64_t minegeld = total_minegeld_in_inventory(node.inventory());
    if (minegeld > 0) {
      const std::string &name = mb.name_for_id(node.param0());
      std::cout << "minegeld: " << std::setw(12) << minegeld << " "
                << NodePos(pos, i).str() << " " << name << "\n";
    }
  }
}

void find_bones(const MapBlockPos &pos, const MapBlock &mb) {
  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];

    const std::string &name = mb.name_for_id(node.param0());
    if (name == "bones:bones") {
      const std::string owner = node.get_meta("_owner");
      std::cout << "bones: " << NodePos(pos, i).str() << " " << owner << "\n";
    }
  }
}

void RunConsumer(const Config &config, MapBlockQueue *queue) {
  std::unique_ptr<MapInterface> map = CreateMapInterface(config);
  Stats stats;

  while (true) {
    const MapBlockKey key = queue->Pop();
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

    if (!mb.deserialize(blob, key.pos)) {
      stats.bad_map_blocks++;
      spdlog::error("Failed to deserialize mapblock {0} {1}", pos.str(),
                    key.pos);
      continue;
    }

    stats.total_map_blocks++;
    stats.by_version[mb.version()]++;

    for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
      const Node &node = mb.nodes()[i];
      const std::string &name = mb.name_for_id(node.param0());

      stats.by_type[name]++;
    }

    find_currency_hoard(pos, mb);
    find_bones(pos, mb);
  }

  dump_stats(stats);
}
