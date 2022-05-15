// Utility to scan every node in a Minetest `map.sqlite` database, looking
// for specific nodes (ex: chest with minegeld).

// Only supports mapblock version 28 ('1c') (minetest-5.4.x, multicraft-2.0.x)
// https://github.com/minetest/minetest/blob/master/doc/world_format.txt

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <istream>
#include <limits>
#include <sstream>
#include <streambuf>
#include <string>
#include <map>
#include <vector>

#include "blob_reader.h"
#include "mapblock.h"
#include "node.h"
#include "pos.h"
#include "utils.h"

struct Stats {
  // Total count of map blocks.
  int64_t total_map_blocks;

  // Count of map blocks that failed to parse.
  int64_t bad_map_blocks;

  // Count of map blocks for each version.
  int64_t by_version[256];

  // Count of nodes of each type.
  std::map<std::string, int64_t> by_type;


  Stats():
    total_map_blocks(0),
    bad_map_blocks(0),
    by_version{},
    by_type()
  {}
};

void dump_stats(const Stats &stats) {
  std::cout << "bad_map_blocks: " << stats.bad_map_blocks << "\n";
  std::cout << "total_map_blocks: " << stats.total_map_blocks << "\n";
  std::cout << "bad %: " << 100.0 * static_cast<double>(stats.bad_map_blocks) /
    static_cast<double>(stats.total_map_blocks) << "\n";

  for (int i = 0; i < 256; i++) {
    if (stats.by_version[i]) {
      std::cout << "version: " << i << " = " << stats.by_version[i] << "\n";
    }
  }

  for (const auto &n: stats.by_type) {
    std::cout << std::setw(12) << n.second << " " << n.first << "\n";
  }
}


void find_currency_hoard(int64_t pos, const MapBlock &mb) {
  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];

    uint64_t minegeld = total_minegeld_in_inventory(node.inventory());
    if (minegeld > 0) {
      const std::string& name = mb.name_for_id(node.param0());
      std::cout << "minegeld: " << std::setw(12) << minegeld << " "
                << Pos(pos, i).str()
                << " " << pos << " " << name << "\n";
    }
  }
}

void find_bones(int64_t pos, const MapBlock &mb) {
  for (size_t i = 0; i < MapBlock::NODES_PER_BLOCK; i++) {
    const Node &node = mb.nodes()[i];

    const std::string& name = mb.name_for_id(node.param0());
    if (name == "bones:bones") {
      const std::string owner = node.get_meta("_owner");
      std::cout << "bones: " << Pos(pos, i).str() << " " << owner << "\n";
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
    const std::string& name = mb.name_for_id(node.param0());

    stats->by_type[name]++;
  }

  find_currency_hoard(pos, mb);
  find_bones(pos, mb);
}


void handle_sqlite3_error(sqlite3* db, int rc) {
  if (rc != SQLITE_OK) {
    std::cout << "Fatal Sqlite error: " << rc << ", " << sqlite3_errstr(rc)
      << "\n";
    exit(-1);
  }
}

void process_file(const char *filename, int64_t min_pos, int64_t max_pos) {
  const char sql[] =
    "select pos, data from blocks "
    "where pos between :min_pos and :max_pos "
    "order by pos";

  sqlite3 *db = nullptr;
  int rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READONLY, nullptr);
  handle_sqlite3_error(db, rc);

  sqlite3_stmt *stmt = nullptr;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  handle_sqlite3_error(db, rc);

  rc = sqlite3_bind_int64(stmt, 1, min_pos);
  handle_sqlite3_error(db, rc);

  rc = sqlite3_bind_int64(stmt, 2, max_pos);
  handle_sqlite3_error(db, rc);

  int64_t blocks = 0;
  Stats stats;
  while (true) {
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
      break;
    }
    blocks++;

    const int64_t pos = sqlite3_column_int64(stmt, 0);
    const uint8_t *data = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1));
    const size_t data_len = sqlite3_column_bytes(stmt, 1);

    // Sadly, we must make a copy... :(
    const std::vector<uint8_t> raw_data(data, data + data_len);
    BlobReader blob(raw_data);

    process_block(pos, blob, &stats);
  }

  rc = sqlite3_finalize(stmt);
  handle_sqlite3_error(db, rc);

  rc = sqlite3_close_v2(db);
  handle_sqlite3_error(db, rc);
  db = nullptr;

//  dump_stats(stats);
}

int main(int argc, char *argv[]) {
  const char *filename = "/dev/null";
  int64_t min_pos = std::numeric_limits<int64_t>::min();
  int64_t max_pos = std::numeric_limits<int64_t>::max();

  Pos::unit_test();

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-min")) {
      min_pos = strtoll(argv[++i], NULL, 10);
    } else if (!strcmp(argv[i], "-max")) {
      max_pos = strtoll(argv[++i], NULL, 10);
    } else if (!strcmp(argv[i], "-pos")) {
      // mapblock pos, not node pos.
      int x = strtol(argv[++i], nullptr, 10);
      int y = strtol(argv[++i], nullptr, 10);
      int z = strtol(argv[++i], nullptr, 10);
      min_pos = max_pos = Pos(x, y, z).mapblock_id_from_node_pos();
    } else {
      filename = argv[i];
    }
  }

  process_file(filename, min_pos, max_pos);

  return 0;
}
