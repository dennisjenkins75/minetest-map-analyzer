// Main application class that holds everything.

#pragma once

#include "src/app/config.h"
#include "src/app/mapblock_queue.h"
#include "src/app/stats.h"
#include "src/lib/id_cache/id_cache.h"

class App {
public:
  App() = delete;
  App(Config &config);
  ~App() {}

  void Run();

private:
  Config config_;
  IdCache actor_ids_;
  IdCache node_ids_;
  MapBlockQueue map_block_queue_;
  Stats stats_;

  void RunProducer();
  void RunConsumer();

  // Primarily for "correctness" debugging, this method runs the entire
  // pipeline serially, on the main thread (no worker threads).
  void RunSerially();
};
