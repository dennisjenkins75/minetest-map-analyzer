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
#include <spdlog/spdlog.h>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "src/app/config.h"
#include "src/app/consumer.h"
#include "src/app/factory.h"
#include "src/app/mapblock_queue.h"
#include "src/app/producer.h"
#include "src/lib/database/db-map-interface.h"
#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"
#include "src/lib/map_reader/utils.h"

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
  spdlog::set_level(spdlog::level::trace);
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
  DebugLogConfig(config);

  if (config.map_filename.empty()) {
    std::cerr << "Error: Must specify path of the map.sqlite file.\n";
    Usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  MapBlockQueue queue;

  RunProducer(config, &queue);
  RunConsumer(config, &queue);

  return 0;
}
