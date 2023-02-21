// Main application class that holds everything.

#pragma once

#include "src/app/actor.h"
#include "src/app/config.h"
#include "src/app/data_writer.h"
#include "src/app/mapblock_queue.h"
#include "src/app/mapblock_writer.h"
#include "src/app/preserve_queue.h"
#include "src/app/stats.h"
#include "src/lib/3dmatrix/3dmatrix.h"
#include "src/lib/id_map/id_map.h"
#include "src/lib/map_reader/mapblock.h"
#include "src/lib/name_filter/name_filter.h"

class App {
public:
  App() = delete;
  App(const Config &config)
      : config_(config), node_filter_(), actor_ids_(),
        node_ids_(
            std::bind(&App::LookupNodeExtraInfo, this, std::placeholders::_1)),
        block_data_(), preserve_queue_(config, block_data_),
        data_writer_(config, node_ids_, actor_ids_),
        map_block_writer_(config, block_data_), map_block_queue_(), stats_(),
        start_time_(std::chrono::steady_clock::now()) {}
  ~App() {}

  void Run();

  void DisplayProgress();

private:
  const Config config_;
  NameFilter node_filter_;
  IdMap<ActorIdMapExtraInfo> actor_ids_;
  IdMap<NodeIdMapExtraInfo> node_ids_;
  Sparse3DMatrix<MapBlockData> block_data_;
  PreserveQueue preserve_queue_;
  DataWriter data_writer_;
  MapBlockWriter map_block_writer_;
  MapBlockQueue map_block_queue_;
  Stats stats_;
  std::chrono::time_point<std::chrono::steady_clock> start_time_;

  // Can be called directly (on main thread), or as a thread body.
  // Exits when all mapblocks have been produced.
  void RunProducer();

  // Can be called directly on main thread, or as a thread body.
  // Will exit when tombstone is observed.
  void RunConsumer();

  // Primarily for "correctness" debugging, this method runs the entire
  // pipeline serially, on the main thread (no worker threads).
  void RunSerially();

  // Run with worker threads.
  void RunThreaded();

  // Preregisteres some super common nodes so that they are first in the
  // node_ids_ map.
  void PreregisterContentIds();

  // Called after primary processing, to apply all of the 'preserve' flags
  // to any map blocks that require it.
  void ApplyPreserveFlags();

  // Called by `node_ids_.Add()` to look up nodes in `node_filter_` to determine
  // if they represent "special" nodes.
  NodeIdMapExtraInfo LookupNodeExtraInfo(const std::string &node_name) {
    return NodeIdMapExtraInfo{
        .anthropocene = node_filter_.Search(node_name),
    };
  }
};
