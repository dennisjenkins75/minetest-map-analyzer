// Class that manages writing large volumes of data to the output sqlite
// database.

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "src/app/actor.h"
#include "src/app/config.h"
#include "src/lib/database/db-sqlite3.h"
#include "src/lib/id_map/id_map.h"
#include "src/lib/map_reader/inventory.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"

// Used by consumer to queue up writes to the `nodes` table in the output
// database.
struct DataWriterNode {
  DataWriterNode() : pos(), owner_id(0), node_id(0), minegeld(0) {}
  NodePos pos;
  size_t owner_id;
  size_t node_id;
  size_t minegeld;
  Inventory inventory;
};

struct DataWriterBlock {
  DataWriterBlock() : pos(), uniform(0), anthropocene(false) {}
  MapBlockPos pos;

  // If the mapblock is 100% the same content_id, then place that here.
  // 0 otherwise.
  uint64_t uniform;

  bool anthropocene;
};

class DataWriter {
public:
  DataWriter() = delete;
  DataWriter(const Config &config, IdMap<NodeIdMapExtraInfo> &node_id_map,
             IdMap<ActorIdMapExtraInfo> &actor_id_map);
  ~DataWriter() {}

  void EnqueueNodes(std::vector<std::unique_ptr<DataWriterNode>> &&nodes) {
    std::unique_lock<std::mutex> lock(node_mutex_);
    for (auto &i : nodes) {
      node_queue_.push(std::move(i));
    }
    node_cv_.notify_one();
  }

  void EnqueueBlock(std::unique_ptr<DataWriterBlock> block) {
    std::unique_lock<std::mutex> lock(block_mutex_);
    block_queue_.push(std::move(block));
    block_cv_.notify_one();
  }

  void FlushActorIdMap();

  void FlushNodeIdMap();

  void FlushNodeQueue();

  void FlushBlockQueue();

  void DataWriterThread();

private:
  const Config &config_;
  IdMap<ActorIdMapExtraInfo> &actor_id_map_;
  IdMap<NodeIdMapExtraInfo> &node_id_map_;

  std::unique_ptr<SqliteDb> database_;
  std::unique_ptr<SqliteStmt> stmt_actor_;
  std::unique_ptr<SqliteStmt> stmt_node_;
  std::unique_ptr<SqliteStmt> stmt_nodes_;
  std::unique_ptr<SqliteStmt> stmt_inventory_;
  std::unique_ptr<SqliteStmt> stmt_blocks_;

  std::queue<std::unique_ptr<DataWriterNode>> node_queue_;
  mutable std::mutex node_mutex_;
  std::condition_variable node_cv_;

  std::queue<std::unique_ptr<DataWriterBlock>> block_queue_;
  mutable std::mutex block_mutex_;
  std::condition_variable block_cv_;
};
