#include <chrono>
#include <list>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "src/app/app.h"

void App::RunSerially() {
  try {
    RunProducer();
    RunConsumer();
    data_writer_.FlushIdMaps();
    data_writer_.FlushNodeQueue();
    stats_.SetTombstone();
    stats_.StatsMergeThread();
  } catch (const Sqlite3Error &err) {
    spdlog::error("Sqlite3Error: {0}", err.what());
  }
}

void App::RunThreaded() {
  std::thread producer_thread(&App::RunProducer, this);

  std::thread stats_merge_thread(&Stats::StatsMergeThread, &stats_);

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

  data_writer_.FlushIdMaps();
  data_writer_.FlushNodeQueue();

  stats_.SetTombstone();
  stats_merge_thread.join();
}

void App::Run() {
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
  spdlog::info("Unique actors: {0}", actor_ids_.size());

  stats_.DumpToFile(node_ids_);
}

// TODO: Read these from a text file.
void App::PreregisterContentIds() {
  node_ids_.Add("ignore"); // 0
  node_ids_.Add("air");    // 1
}
