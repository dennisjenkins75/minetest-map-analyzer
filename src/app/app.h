// Main application class that holds everything.

#pragma once

#include "src/app/config.h"
#include "src/app/mapblock_queue.h"
#include "src/app/stats.h"
#include "src/lib/id_map/id_map.h"

class App {
public:
  App() = delete;
  App(const Config &config)
      : config_(config), actor_ids_(), node_ids_(), map_block_queue_(),
        stats_() {}
  ~App() {}

  void Run();

private:
  const Config config_;
  IdMap actor_ids_;
  IdMap node_ids_;
  MapBlockQueue map_block_queue_;
  Stats stats_;

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
};
