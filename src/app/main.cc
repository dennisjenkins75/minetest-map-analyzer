// Utility to scan every node in a Minetest `map.sqlite` database, looking
// for specific nodes (ex: chest with minegeld).

// Only supports mapblock version 28 ('1c') (minetest-5.4.x, multicraft-2.0.x)
// https://github.com/minetest/minetest/blob/master/doc/world_format.txt

#include <arpa/inet.h>
#include <getopt.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "src/app/config.h"
#include "src/app/factory.h"
#include "src/app/mapblock_queue.h"
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

void find_currency_hoard(int64_t pos, const MapBlock &mb) {
  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];

    uint64_t minegeld = total_minegeld_in_inventory(node.inventory());
    if (minegeld > 0) {
      const std::string &name = mb.name_for_id(node.param0());
      std::cout << "minegeld: " << std::setw(12) << minegeld << " "
                << NodePos(pos, i).str() << " " << pos << " " << name << "\n";
    }
  }
}

void find_bones(int64_t pos, const MapBlock &mb) {
  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];

    const std::string &name = mb.name_for_id(node.param0());
    if (name == "bones:bones") {
      const std::string owner = node.get_meta("_owner");
      std::cout << "bones: " << NodePos(pos, i).str() << " " << owner << "\n";
    }
  }
}

void process_block(int64_t pos, BlobReader &blob, Stats *stats) {
  MapBlock mb;

  if (!mb.deserialize(blob, pos)) {
    stats->bad_map_blocks++;
    std::cout << "Bad Block: " << RED << pos << CLEAR << "\n";
    exit(-1);
  }
  stats->total_map_blocks++;
  stats->by_version[mb.version()]++;

  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];
    const std::string &name = mb.name_for_id(node.param0());

    stats->by_type[name]++;
  }

  find_currency_hoard(pos, mb);
  find_bones(pos, mb);
}

void process_file(const Config &config) {
  MapBlockQueue queue;
  std::unique_ptr<MapInterface> map = CreateMapInterface(config);

  const auto callback = [&queue](int64_t id, int64_t mtime) -> bool {
    queue.Enqueue(std::move(MapBlockKey(id, mtime)));
    return true;
  };

  map->ProduceMapBlocks(config.min_pos, config.max_pos, callback);
  std::cout << "MapBlocks: " << queue.size() << "\n";
  queue.SetTombstone();

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

  //  dump_stats(stats);
}

static const int OPT_MIN = 257;
static const int OPT_MAX = 258;
static const int OPT_POS = 259;

static struct option long_options[] = {
    {"min", required_argument, NULL, OPT_MIN},
    {"max", required_argument, NULL, OPT_MAX},
    {"pos", required_argument, NULL, OPT_POS},
    {"threads", required_argument, NULL, 't'},
    {"max_load_avg", required_argument, NULL, 'l'},
    {NULL, 0, NULL, 0}};

void Usage(const char *prog) {
  std::cerr << "Usage: " << prog << " [options] filename\n";
  std::cerr << "Options: \n";
  std::cerr << "  --min   x,y,z - Min mapblock to examine.\n";
  std::cerr << "  --max   x,y,z - Max mapblock to examine.\n";
  std::cerr << "  --pos   x,y,z - Only mapblock to examine.\n";
  std::cerr << "  --threads n   - Max count of consumer threads.\n";
  std::cerr << "  --max_load_avg n - Max load average to allow.\n";
}

int main(int argc, char *argv[]) {
  Config config;

  while (true) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "l:t:", long_options, &option_index);

    if (c < 0) {
      break;
    }

    switch (c) {
      case 0:
        std::cout << "option " << long_options[option_index].name;
        if (optarg) {
          std::cout << " with arg " << optarg;
        }
        std::cout << "\n";
        break;

      case 'l':
        config.max_load_avg = strtod(optarg, NULL);
        break;

      case 't':
        config.threads = strtol(optarg, NULL, 10);
        break;

      case OPT_MIN:
        if (!config.min_pos.parse(optarg)) {
          std::cerr << "ERROR: Invalid MapBlockPos value: " << optarg << "\n";
          exit(EXIT_FAILURE);
        }
        break;

      case OPT_MAX:
        if (!config.max_pos.parse(optarg)) {
          std::cerr << "ERROR: Invalid MapBlockPos value: " << optarg << "\n";
          exit(EXIT_FAILURE);
        }
        break;

      case OPT_POS:
        if (!config.min_pos.parse(optarg)) {
          std::cerr << "ERROR: Invalid MapBlockPos value: " << optarg << "\n";
          exit(EXIT_FAILURE);
        }
        config.max_pos = MapBlockPos(config.min_pos.x + 1, config.min_pos.y + 1,
                                     config.min_pos.z + 1);
        break;

      default:
        std::cerr << "Unrecognized getopt_long() result of " << c << "\n";
        break;
    }
  }

  if (optind < argc) {
    // Process non-option ARGV elements (filenames)
    config.map_filename = argv[optind++];
  }

  config.min_pos.sort(&config.max_pos);

  std::cout << "min_pos: " << config.min_pos.str() << " "
            << config.min_pos.MapBlockId() << "\n";
  std::cout << "max_pos: " << config.max_pos.str() << " "
            << config.max_pos.MapBlockId() << "\n";
  std::cout << "threads: " << config.threads << "\n";
  std::cout << "max_load_avg: " << config.max_load_avg << "\n";

  if (config.map_filename.empty()) {
    std::cerr << "Error: Must specify path of the map.sqlite file.\n";
    Usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  process_file(config);

  return 0;
}
