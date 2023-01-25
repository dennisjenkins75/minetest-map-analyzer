#include "src/lib/database/db-map-postgresql.h"
#include "src/lib/exceptions/exceptions.h"

static constexpr char kStmtProduceMapBlocks[] = "produceMapBlocks";
static constexpr char kSqlProduceMapBlocks[] = R"sql(
select posx, posy, posz
from blocks
where (posx between $1 and $2)
  and (posy between $3 and $4)
  and (posz between $5 and $6)
)sql";

static constexpr char kStmtLoadMapBlock[] = "loadMapBlock";
static constexpr char kSqlLoadMapBlock[] = R"sql(
select data
from blocks
where (posx = $1) and (posy = $2) and (posz = $3)
)sql";

MapInterfacePostgresql::MapInterfacePostgresql(
    const std::string &connection_str)
    : pqxx_url_(connection_str), connection_() {
  connection_ = std::make_unique<pqxx::connection>(connection_str);

  connection_->prepare(kStmtProduceMapBlocks, kSqlProduceMapBlocks);
  connection_->prepare(kStmtLoadMapBlock, kSqlLoadMapBlock);
}

std::optional<MapInterface::Blob>
MapInterfacePostgresql::LoadMapBlock(const MapBlockPos &pos) {
  pqxx::work xact(*connection_, __FUNCTION__);
  const auto result =
      xact.exec_prepared(kStmtLoadMapBlock, pos.x, pos.y, pos.z);
  if (result.empty()) {
    return std::nullopt; // block not found.
  }

  const pqxx::binarystring bin = result[0][0].as<pqxx::binarystring>();
  const uint8_t *data = static_cast<const uint8_t *>(bin.data());
  return std::vector<uint8_t>(data, data + bin.size());
}

bool MapInterfacePostgresql::ProduceMapBlocks(
    const MapBlockPos &min, const MapBlockPos &max,
    std::function<bool(const MapBlockPos &, int64_t)> callback) {
  pqxx::work xact(*connection_, __FUNCTION__);
  const auto result = xact.exec_prepared(kStmtProduceMapBlocks, min.x, max.x,
                                         min.y, max.y, min.z, max.z);
  for (const auto &row : result) {
    const int x = row["posx"].as<int64_t>();
    const int y = row["posy"].as<int64_t>();
    const int z = row["posz"].as<int64_t>();
    const int mtime = 0; // TODO: extract mtime if it exists?

    MapBlockPos pos(x, y, z);
    if (!pos.inside(min, max)) {
      continue;
    }
    if (!callback(pos, mtime)) {
      return false;
    }
  }

  return true;
}

void MapInterfacePostgresql::DeleteMapBlocks(
    const std::vector<MapBlockPos> &list) {
  throw UnimplementedError("MapInterfacePostgresql::DeleteMapBlocks");
}
