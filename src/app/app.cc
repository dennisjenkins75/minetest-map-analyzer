#include <chrono>
#include <list>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "src/app/app.h"
#include "src/app/schema/schema.h"

App::App(Config &config)
    : config_(config), actor_ids_(), node_ids_(), map_block_queue_(), stats_() {
}

void App::RunSerially() {
  RunProducer();
  RunConsumer();
}

void App::RunThreaded() {
  std::thread producer_thread(&App::RunProducer, this);

  std::vector<std::thread> consumer_threads;
  consumer_threads.reserve(config_.threads);
  for (int i = 0; i < config_.threads; ++i) {
    consumer_threads.push_back(std::thread(&App::RunConsumer, this));
  }

  spdlog::trace("Main thread waiting for worker to finish");

  producer_thread.join();
  for (auto &t : consumer_threads) {
    t.join();
  }
}

void App::Run() {
  VerifySchema(config_.data_filename);

  PreregisterContentIds();

  const auto t0 = std::chrono::steady_clock::now();
  if (config_.threads) {
    RunThreaded();
  } else {
    RunSerially();
  }
  const auto t1 = std::chrono::steady_clock::now();

  const std::chrono::duration<double> diff = t1 - t0;
  const double rate = stats_.TotalBlocks() / diff.count();
  spdlog::info("Processed {0} blocks in {1:.2f} seconds via {2} threads for "
               "{3:.2f} blocks/sec, {4:.2f} blocks/sec/thread.",
               stats_.TotalBlocks(), diff.count(), config_.threads, rate,
               rate / config_.threads);
  spdlog::info("Unique nodes: {0}", node_ids_.size());

  stats_.DumpToFile(node_ids_);
}

// TODO: Read these from a text file.
void App::PreregisterContentIds() {
  node_ids_.Add("ignore"); // 0
  node_ids_.Add("air");    // 1
}
