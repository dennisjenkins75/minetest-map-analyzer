#include <chrono>
#include <spdlog/spdlog.h>

#include "src/app/app.h"
#include "src/app/schema/schema.h"

App::App(Config &config)
    : config_(config), actor_ids_(), node_ids_(), map_block_queue_(), stats_() {
}

void App::Run() {
  VerifySchema(config_.data_filename);

  const auto t0 = std::chrono::steady_clock::now();

  RunProducer();
  RunConsumer();

  const auto t1 = std::chrono::steady_clock::now();

  const std::chrono::duration<double> diff = t1 - t0;
  const double rate = stats_.TotalBlocks() / diff.count();
  spdlog::info("Processed {0} blocks in {1:.2f} seconds.  {2:.2f} blocks/sec.",
               stats_.TotalBlocks(), diff.count(), rate);

  stats_.DumpToFile();
}
