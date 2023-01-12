#include "src/app/data_writer.h"
#include "src/app/schema/schema.h"

static constexpr char kSqlWriteActor[] = R"sql(
  insert into actor (actor_id, name) values (:id, :name)
)sql";

static constexpr char kSqlWriteNode[] = R"sql(
  insert into node (node_id, name) values (:id, :name)
)sql";

DataWriter::DataWriter(const Config &config, IdMap &node_id_map,
                       IdMap &actor_id_map)
    : config_(config), actor_id_map_(actor_id_map), node_id_map_(node_id_map),
      database_() {
  VerifySchema(config_.data_filename);

  database_ = std::make_unique<SqliteDb>(config.data_filename);
  stmt_actor_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteActor);
  stmt_node_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteNode);
}

void DataWriter::FlushIdMaps() {
  FlushDirtyIdMap(stmt_actor_.get(), actor_id_map_.GetDirty());
  FlushDirtyIdMap(stmt_node_.get(), node_id_map_.GetDirty());
}

void DataWriter::FlushDirtyIdMap(SqliteStmt *stmt,
                                 const IdMap::DirtyList &list) {
  if (list.empty())
    return;

  database_->Begin();
  for (const auto &item : list) {
    stmt->BindInt(1, item.first);
    stmt->BindText(2, item.second);
    stmt->Step();
    stmt->Reset();
  }
  database_->Commit();
}
