#include "src/app/mapblock_writer.h"
#include "src/app/schema/schema.h"

static constexpr char kSqlWriteBlock[] = R"sql(
  insert into blocks
    (mapblock_id, mapblock_x, mapblock_y, mapblock_z, uniform, anthropocene)
  values
    (:mapblock_id, :mapblock_x, :mapblock_y, :mapblock_z, :uniform,
     :anthropocene)
)sql";

MapBlockWriter::MapBlockWriter(const Config &config,
                               Sparse3DMatrix<MapBlockData> &block_data)
    : config_(config), block_data_(block_data), stmt_blocks_(), block_queue_(),
      block_mutex_(), block_cv_() {
  VerifySchema(config_.out_filename);

  database_ = std::make_unique<SqliteDb>(config.out_filename);
  stmt_blocks_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteBlock);
}

void MapBlockWriter::FlushBlockQueue() {
  std::unique_lock<std::mutex> lock(block_mutex_);

  if (block_queue_.empty())
    return;

  database_->Begin();
  while (!block_queue_.empty()) {
    const MapBlockPos pos = block_queue_.front();
    block_queue_.pop();

    const MapBlockData &block = block_data_.Ref(pos.x, pos.y, pos.z);

    stmt_blocks_->BindInt(1, block.pos.MapBlockId());
    stmt_blocks_->BindInt(2, block.pos.x);
    stmt_blocks_->BindInt(3, block.pos.y);
    stmt_blocks_->BindInt(4, block.pos.z);
    stmt_blocks_->BindInt(5, block.uniform);
    stmt_blocks_->BindBool(6, block.anthropocene);
    stmt_blocks_->Step();
    stmt_blocks_->Reset();
  }
  database_->Commit();
}
