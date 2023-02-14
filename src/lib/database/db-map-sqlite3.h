// Sqlite3 implementation for `class MapInterface`.

#pragma once

#include "src/lib/database/db-map-interface.h"
#include "src/lib/database/db-sqlite3.h"

class MapInterfaceSqlite3 : public MapInterface {
public:
  MapInterfaceSqlite3() = delete;

  explicit MapInterfaceSqlite3(std::string_view connection_str);
  virtual ~MapInterfaceSqlite3() {}

  std::optional<Blob> LoadMapBlock(const MapBlockPos &pos) override;

  bool
  ProduceMapBlocks(const MapBlockPos &min, const MapBlockPos &max,
                   std::function<bool(const MapBlockPos &)> callback) override;

  void DeleteMapBlocks(const std::vector<MapBlockPos> &list) override;

protected:
  std::unique_ptr<SqliteDb> db_;
  std::unique_ptr<SqliteStmt> stmt_load_block_;
  std::unique_ptr<SqliteStmt> stmt_list_blocks_;
  std::unique_ptr<SqliteStmt> stmt_delete_block_;
};
