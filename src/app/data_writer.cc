#include "src/app/data_writer.h"
#include "src/app/schema/schema.h"

static constexpr char kSqlWriteActor[] = R"sql(
  insert into actor (actor_id, name) values (:id, :name)
)sql";

static constexpr char kSqlWriteNode[] = R"sql(
  insert into node (node_id, name, special) values (:id, :name, :special)
)sql";

static constexpr char kSqlWriteNodes[] = R"sql(
  insert into nodes (pos_id, node_x, node_y, node_z, owner_id, node_id,
                     minegeld)
  values (:pos_id, :node_x, :node_y, :node_z, :owner_id, :node_id, :minegeld)
)sql";

static constexpr char kSqlWriteInventory[] = R"sql(
  insert into inventory (pos_id, type, item_string)
  values (:pos_id, :type, :item_string)
)sql";

DataWriter::DataWriter(const Config &config,
                       IdMap<NodeIdMapExtraInfo> &node_id_map,
                       IdMap<ActorIdMapExtraInfo> &actor_id_map)
    : config_(config), actor_id_map_(actor_id_map), node_id_map_(node_id_map),
      database_(), stmt_actor_(), stmt_node_(), stmt_nodes_(),
      stmt_inventory_(), node_queue_(), node_mutex_(), node_cv_() {
  VerifySchema(config_.out_filename);

  database_ = std::make_unique<SqliteDb>(config.out_filename);
  stmt_actor_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteActor);
  stmt_node_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteNode);
  stmt_nodes_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteNodes);
  stmt_inventory_ =
      std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteInventory);
}

void DataWriter::FlushActorIdMap() {
  DirtyList dirty_list = actor_id_map_.GetDirty();

  if (dirty_list.empty())
    return;

  database_->Begin();
  for (const auto &item : dirty_list) {
    stmt_actor_->BindInt(1, item.first);
    stmt_actor_->BindText(2, item.second);
    stmt_actor_->Step();
    stmt_actor_->Reset();
  }
  database_->Commit();
}

void DataWriter::FlushNodeIdMap() {
  DirtyList dirty_list = node_id_map_.GetDirty();

  if (dirty_list.empty())
    return;

  database_->Begin();
  for (const auto &item : dirty_list) {
    stmt_node_->BindInt(1, item.first);
    stmt_node_->BindText(2, item.second);
    stmt_node_->BindBool(3, node_id_map_.Get(item.first).extra.anthropocene);
    stmt_node_->Step();
    stmt_node_->Reset();
  }
  database_->Commit();
}

void DataWriter::FlushNodeQueue() {
  if (node_queue_.empty())
    return;

  database_->Begin();
  while (!node_queue_.empty()) {
    std::unique_ptr<DataWriterNode> node = std::move(node_queue_.front());
    node_queue_.pop();

    const uint64_t pos_id = node->pos.NodePosId();
    stmt_nodes_->BindInt(1, pos_id);
    stmt_nodes_->BindInt(2, node->pos.x);
    stmt_nodes_->BindInt(3, node->pos.y);
    stmt_nodes_->BindInt(4, node->pos.z);
    stmt_nodes_->BindInt(5, node->owner_id);
    stmt_nodes_->BindInt(6, node->node_id);
    stmt_nodes_->BindInt(7, node->minegeld);
    stmt_nodes_->Step();
    stmt_nodes_->Reset();

    if (!node->inventory.empty()) {
      for (const auto &list : node->inventory.lists()) {
        for (const auto &item : list.second.items()) {
          if (!item.empty()) {
            stmt_inventory_->BindInt(1, pos_id);
            stmt_inventory_->BindText(2, list.first);
            stmt_inventory_->BindText(3, item);
            stmt_inventory_->Step();
            stmt_inventory_->Reset();
          }
        }
      }
    }
  }
  database_->Commit();
}
