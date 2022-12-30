#include "src/lib/database/db-map-sqlite3.h"

static constexpr char kSqlLoadBlock[] = R"sql(
select data from blocks where pos = :pos
)sql";

static constexpr char kSqlListBlocks[] = R"sql(
select pos, mtime
from blocks
where pos between :min_pos and :max_pos
)sql";

static constexpr char kSqlDeleteBlock[] = R"sql(
delete from blocks where pos = :pos
)sql";

MapInterfaceSqlite3::MapInterfaceSqlite3(std::string_view connection_str)
    : db_(), stmt_load_block_(), stmt_list_blocks_(), stmt_delete_block_() {
  db_ = std::make_unique<DatabaseSqlite3>(connection_str);

  stmt_load_block_ = db_->Prepare(kSqlLoadBlock);
  stmt_list_blocks_ = db_->Prepare(kSqlListBlocks);
  stmt_delete_block_ = db_->Prepare(kSqlDeleteBlock);
}

std::optional<MapInterface::Blob>
MapInterfaceSqlite3::LoadMapBlock(const MapBlockPos &pos) {
  const auto stmt = stmt_load_block_.get();
  db_->BindInt(stmt, 1, pos.MapBlockId());

  if (!db_->Step(stmt)) {
    db_->Reset(stmt);
    return std::nullopt; // block not found.
  }

  Blob blob = db_->ColumnBlob(stmt, 0);
  db_->Reset(stmt);
  return blob;
}

void MapInterfaceSqlite3::DeleteMapBlocks(
    const std::vector<MapBlockPos> &list) {
  throw UnimplementedError("MapInterfaceSqlite3::DeleteMapBlocks");
}

bool MapInterfaceSqlite3::ProduceMapBlocks(
    const MapBlockPos &min, const MapBlockPos &max,
    std::function<bool(int64_t, int64_t)> callback) {
  const auto stmt = stmt_list_blocks_.get();
  db_->BindInt(stmt, 1, min.MapBlockId());
  db_->BindInt(stmt, 2, max.MapBlockId());

  while (db_->Step(stmt)) {
    const int64_t pos = db_->ColumnInt64(stmt, 0);
    const int64_t mtime = db_->ColumnInt64(stmt, 1);

    if (!MapBlockPos(pos).inside(min, max)) {
      continue;
    }

    if (!callback(pos, mtime)) {
      return false;
    }
  }

  db_->Reset(stmt);
  return true;
}
