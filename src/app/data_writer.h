// Class that manages writing large volumes of data to the output sqlite
// database.

#pragma once

#include "src/app/config.h"
#include "src/lib/database/db-sqlite3.h"
#include "src/lib/id_map/id_map.h"

class DataWriter {
public:
  DataWriter() = delete;
  DataWriter(const Config &config, IdMap &node_id_map, IdMap &actor_id_map);
  ~DataWriter() {}

  void FlushIdMaps();

  void DataWriterThread();

private:
  const Config &config_;
  IdMap &actor_id_map_;
  IdMap &node_id_map_;

  std::unique_ptr<SqliteDb> database_;
  std::unique_ptr<SqliteStmt> stmt_actor_;
  std::unique_ptr<SqliteStmt> stmt_node_;

  void FlushDirtyIdMap(SqliteStmt *stmt, const IdMap::DirtyList &list);
};
