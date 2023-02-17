// Class that manages writing large volumes of data to the output sqlite
// database.

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "src/app/actor.h"
#include "src/app/config.h"
#include "src/lib/3dmatrix/3dmatrix.h"
#include "src/lib/database/db-sqlite3.h"
#include "src/lib/id_map/id_map.h"
#include "src/lib/map_reader/inventory.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"

struct MapBlockData {
  MapBlockData() : pos(), uniform(0), anthropocene(false) {}
  MapBlockPos pos;

  // If the mapblock is 100% the same content_id, then place that here.
  // 0 otherwise.
  uint64_t uniform;

  bool anthropocene;
};

class MapBlockWriter {
public:
  MapBlockWriter() = delete;
  MapBlockWriter(const Config &config,
                 Sparse3DMatrix<MapBlockData> &block_data);
  ~MapBlockWriter() {}

  void EnqueueMapBlockPos(const MapBlockPos &pos) {
    std::unique_lock<std::mutex> lock(block_mutex_);
    block_queue_.push(pos);
    block_cv_.notify_one();
  }

  // Should only be called AFTER all analysis is done.
  // Because analyzing one mapblock might update data on its neighbors.
  void FlushBlockQueue();

private:
  const Config &config_;
  Sparse3DMatrix<MapBlockData> &block_data_;

  std::unique_ptr<SqliteDb> database_;
  std::unique_ptr<SqliteStmt> stmt_blocks_;

  std::queue<MapBlockPos> block_queue_;
  mutable std::mutex block_mutex_;
  std::condition_variable block_cv_;
};
