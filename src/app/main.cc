// Utility to scan every node in a Minetest `map.sqlite` database, looking
// for specific nodes (ex: chest with minegeld).

// Only supports mapblock version 28 and 29.
// https://github.com/minetest/minetest/blob/master/doc/world_format.txt

#include <getopt.h>
#include <string.h>

#include <iostream>
#include <limits>
#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/config.h"

static constexpr int OPT_MIN = 257;
static constexpr int OPT_MAX = 258;
static constexpr int OPT_POS = 259;
static constexpr int OPT_MAP = 260;
static constexpr int OPT_OUT = 261;
static constexpr int OPT_HELP = 262;
static constexpr int OPT_DRIVER = 263;
static constexpr int OPT_PATTERN = 264;
static constexpr int OPT_RADIUS = 265;
static constexpr int OPT_STATS = 266;
static constexpr int OPT_MINEGELD = 267;

static struct option long_options[] = {
    {"help", no_argument, NULL, OPT_HELP},
    {"min", required_argument, NULL, OPT_MIN},
    {"max", required_argument, NULL, OPT_MAX},
    {"pos", required_argument, NULL, OPT_POS},
    {"map", required_argument, NULL, OPT_MAP},
    {"out", required_argument, NULL, OPT_OUT},
    {"pattern", required_argument, NULL, OPT_PATTERN},
    {"driver", required_argument, NULL, OPT_DRIVER},
    {"radius", required_argument, NULL, OPT_RADIUS},
    {"threads", required_argument, NULL, 't'},
    {"max_load_avg", required_argument, NULL, 'l'},
    {"stats", required_argument, NULL, OPT_STATS},
    {"minegeld", no_argument, NULL, OPT_MINEGELD},
    {NULL, 0, NULL, 0}};

void Usage(const char *prog) {
  std::cerr
      << "Usage: " << prog << " [options]\n"
      << "Options: \n"
      << "  --help           - Display this help.\n"
      << "  --min   x,y,z    - Min mapblock to examine.\n"
      << "  --max   x,y,z    - Max mapblock to examine.\n"
      << "  --pos   x,y,z    - Only mapblock to examine.\n"
      << "  --threads n      - Max count of consumer threads.\n"
      << "  --max_load_avg n - Max load average to allow.\n"
      << "  --driver type    - Map reader driver (sqlite or postgresql).\n"
      << "  --map   filename - Path to map.sqlite file (REQUIRED).\n"
      << "  --out   filename - Path to output sqlite file (REQUIRED).\n"
      << "  --pattern filename - Path to node name regex list (optional).\n"
      << "  --stats filename - Path to append runtime stats to.\n"
      << "  --radius n       - Mapblock radius to preserve. See README file.\n"
      << "  --minegeld       - Track per-node minegeld amounts.\n"
      << "";
}

int main(int argc, char *argv[]) {
  spdlog::set_level(spdlog::level::info);
  Config config;

  // Threads == 0 means "all on main thread" and "no progress bar".
  // To get the progress bar, we must have at least 1 consumer thread.
  config.threads = 1;

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

      case OPT_HELP:
        Usage(argv[0]);
        exit(EXIT_SUCCESS);
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

      case OPT_DRIVER:
        if (!strcmp(optarg, "sqlite")) {
          config.driver_type = MapDriverType::SQLITE;
        } else if (!strcmp(optarg, "postgresql") || !strcmp(optarg, "pgsql")) {
          config.driver_type = MapDriverType::POSTGRESQL;
        } else {
          std::cerr << "ERROR: Invalid driver value: " << optarg << "\n";
          exit(EXIT_FAILURE);
        }
        break;

      case OPT_MAP:
        config.map_filename = optarg;
        break;

      case OPT_OUT:
        config.out_filename = optarg;
        break;

      case OPT_PATTERN:
        config.pattern_filename = optarg;
        break;

      case OPT_STATS:
        config.stats_filename = optarg;
        break;

      case OPT_RADIUS:
        config.preserve_radius = strtoul(optarg, NULL, 10);
        break;

      case OPT_MINEGELD:
        config.track_minegeld = true;
        break;

      default:
        std::cerr << "Unrecognized getopt_long() result of " << c << "\n";
        break;
    }
  }

  if (optind < argc) {
    std::cerr << "Unrecognized extra command line args\n";
    Usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  config.min_pos.sort(&config.max_pos);
  DebugLogConfig(config);

  if (config.map_filename.empty()) {
    std::cerr << "Error: required argument '--map' is missing.\n";
    Usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  if (config.out_filename.empty()) {
    std::cerr << "Error: required argument '--out' is missing.\n";
    Usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // If the output database already exists, we must clear it out.  Currently,
  // we do not support "resuming an aborted analysis", so running twice with
  // the same output database will result in primary key violations.
  // Ok to ignore failure if file does not exist.
  unlink(config.out_filename.c_str());

  App app(config);
  app.Run();

  return 0;
}
