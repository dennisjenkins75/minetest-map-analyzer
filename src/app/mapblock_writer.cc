#include <spdlog/spdlog.h>
#include <unordered_set>

#include "src/app/mapblock_writer.h"
#include "src/app/schema/schema.h"

static constexpr char kSqlWriteBlock[] = R"sql(
  insert into blocks
    (mapblock_id, mapblock_x, mapblock_y, mapblock_z, uniform, anthropocene,
     preserve)
  values
    (:mapblock_id, :mapblock_x, :mapblock_y, :mapblock_z, :uniform,
     :anthropocene, :preserve)
)sql";

struct Hash {
  size_t operator()(const MapBlockPos &pos) const {
    return static_cast<size_t>(pos.MapBlockId());
  }
};

MapBlockWriter::MapBlockWriter(const Config &config,
                               Sparse3DMatrix<MapBlockData> &block_data)
    : config_(config), block_data_(block_data), stmt_blocks_(), block_queue_(),
      block_mutex_(), block_cv_() {
  VerifySchema(config_.out_filename);

  database_ = std::make_unique<SqliteDb>(config.out_filename);
  stmt_blocks_ = std::make_unique<SqliteStmt>(*database_.get(), kSqlWriteBlock);
}

void MapBlockWriter::PreserveAdjacentBlocks() {
  spdlog::trace("PreserveAdjacentBlocks enter");

  std::unique_lock<std::mutex> lock(block_mutex_);
  std::unordered_set<MapBlockPos, Hash> merged;

  for (auto &pos : block_queue_) {
    const MapBlockData &block = block_data_.Ref(pos.x, pos.y, pos.z);

    if (block.anthropocene) {
      const int r = config_.preserve_radius; // alias for brevity.

      // Mark adjacent blocks as "don't delete".
      for (int z = pos.z - r; z <= pos.z + r; ++z) {
        for (int y = pos.y - r; y <= pos.y + r; ++y) {
          for (int x = pos.x - r; x <= pos.x + r; ++x) {
            merged.emplace(MapBlockPos(x, y, z));
          }
        }
      }
    }
  }

  for (auto &pos : merged) {
    block_data_.Ref(pos.x, pos.y, pos.z).preserve = true;
  }

  spdlog::trace("PreserveAdjacentBlocks exit");
}

void MapBlockWriter::FlushBlockQueue() {
  spdlog::trace("FlushBlockQueue enter");

  std::unique_lock<std::mutex> lock(block_mutex_);

  database_->Begin();
  while (!block_queue_.empty()) {
    const MapBlockPos pos = block_queue_.front();
    block_queue_.pop_front();

    const MapBlockData &block = block_data_.Ref(pos.x, pos.y, pos.z);

    stmt_blocks_->BindInt(1, block.pos.MapBlockId());
    stmt_blocks_->BindInt(2, block.pos.x);
    stmt_blocks_->BindInt(3, block.pos.y);
    stmt_blocks_->BindInt(4, block.pos.z);
    stmt_blocks_->BindInt(5, block.uniform);
    stmt_blocks_->BindBool(6, block.anthropocene);
    stmt_blocks_->BindBool(7, block.preserve);
    stmt_blocks_->Step();
    stmt_blocks_->Reset();
  }
  database_->Commit();

  spdlog::trace("FlushBlockQueue exit");
}
