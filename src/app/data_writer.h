// Class that manages writing large volumes of data to the output sqlite
// database.

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "src/app/config.h"
#include "src/lib/database/db-sqlite3.h"
#include "src/lib/id_map/id_map.h"
#include "src/lib/map_reader/inventory.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/pos.h"

// Used by consumer to queue up writes to the `nodes` table in the output
// database.
struct DataWriterNode {
  DataWriterNode() : pos(), owner_id(-1), node_id(0), minegeld(0) {}
  NodePos pos;
  size_t owner_id;
  size_t node_id;
  size_t minegeld;
  Inventory inventory;
};

class DataWriter {
public:
  DataWriter() = delete;
  DataWriter(const Config &config, IdMap &node_id_map, IdMap &actor_id_map);
  ~DataWriter() {}

  void EnqueueNodes(std::vector<std::unique_ptr<DataWriterNode>> &&nodes) {
    std::unique_lock<std::mutex> lock(node_mutex_);
    for (auto &i : nodes) {
      node_queue_.push(std::move(i));
    }
    node_cv_.notify_one();
  }

  void FlushIdMaps();

  void FlushNodeQueue();

  void DataWriterThread();

private:
  const Config &config_;
  IdMap &actor_id_map_;
  IdMap &node_id_map_;

  std::unique_ptr<SqliteDb> database_;
  std::unique_ptr<SqliteStmt> stmt_actor_;
  std::unique_ptr<SqliteStmt> stmt_node_;
  std::unique_ptr<SqliteStmt> stmt_nodes_;
  std::unique_ptr<SqliteStmt> stmt_inventory_;

  std::queue<std::unique_ptr<DataWriterNode>> node_queue_;
  mutable std::mutex node_mutex_;
  std::condition_variable node_cv_;

  void FlushDirtyIdMap(SqliteStmt *stmt, const IdMap::DirtyList &list);
};
