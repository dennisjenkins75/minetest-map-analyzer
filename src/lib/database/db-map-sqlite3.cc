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
  db_ = std::make_unique<SqliteDb>(connection_str);

  stmt_load_block_ = std::make_unique<SqliteStmt>(*db_.get(), kSqlLoadBlock);
  stmt_list_blocks_ = std::make_unique<SqliteStmt>(*db_.get(), kSqlListBlocks);
  stmt_delete_block_ =
      std::make_unique<SqliteStmt>(*db_.get(), kSqlDeleteBlock);
}

std::optional<MapInterface::Blob>
MapInterfaceSqlite3::LoadMapBlock(const MapBlockPos &pos) {
  stmt_load_block_->BindInt(1, pos.MapBlockId());

  if (!stmt_load_block_->Step()) {
    stmt_load_block_->Reset();
    return std::nullopt; // block not found.
  }

  Blob blob = stmt_load_block_->ColumnBlob(0);
  stmt_load_block_->Reset();
  return blob;
}

void MapInterfaceSqlite3::DeleteMapBlocks(
    const std::vector<MapBlockPos> &list) {
  throw UnimplementedError("MapInterfaceSqlite3::DeleteMapBlocks");
}

bool MapInterfaceSqlite3::ProduceMapBlocks(
    const MapBlockPos &min, const MapBlockPos &max,
    std::function<bool(int64_t, int64_t)> callback) {

  stmt_list_blocks_->BindInt(1, min.MapBlockId());
  stmt_list_blocks_->BindInt(2, max.MapBlockId());

  while (stmt_list_blocks_->Step()) {
    const int64_t pos = stmt_list_blocks_->ColumnInt64(0);
    const int64_t mtime = stmt_list_blocks_->ColumnInt64(1);

    if (!MapBlockPos(pos).inside(min, max)) {
      continue;
    }

    if (!callback(pos, mtime)) {
      return false;
    }
  }

  stmt_load_block_->Reset();
  return true;
}
