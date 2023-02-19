#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <spdlog/spdlog.h>
#include <sys/resource.h>
#include <thread>
#include <vector>

#include "src/app/app.h"

static constexpr auto kProgressInterval = std::chrono::milliseconds(100);
static constexpr std::string_view kColorReset = "\x1b[0m";
static constexpr std::string_view kColorLabel = "\x1b[0m"; // default
static constexpr std::string_view kColorData = "\x1b[32m"; // green
static constexpr std::string_view kClearEOL = "\x1b[0K";
static constexpr std::string_view kCursorLeft = "\x1b[0G";

void App::DisplayProgress() {
  // Warning.. Linux/xterm specific escape codes.
  // TODO: Maybe port to using ncurses?
  // https://www.xfree86.org/current/ctlseqs.html
  // https://en.wikipedia.org/wiki/ANSI_escape_code

  StatsData data = stats_.Get();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> time_diff = now - start_time_;

  // Avoid divide by zero.
  if (!time_diff.count() || !data.queued_map_blocks_) {
    return;
  }

  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) < 0) {
    return;
  }

  const uint64_t sofar = data.good_map_blocks_ + data.bad_map_blocks_;
  const uint64_t remaining = data.queued_map_blocks_ - sofar;
  const double perc =
      100.0 * static_cast<double>(sofar) / data.queued_map_blocks_;
  const double blocks_per_second = sofar / time_diff.count();
  const double eta = remaining / blocks_per_second;

  std::stringstream ss;

  // Return cursor to left side, same line.
  ss << kCursorLeft;

  // Colors!
  ss << kColorData << sofar << kColorLabel << " of " << kColorData
     << data.queued_map_blocks_ << kColorLabel << "  (" << std::fixed
     << std::setprecision(3) << kColorData << perc << kColorLabel
     << "%)  blocks/s: " << std::setprecision(1) << kColorData
     << blocks_per_second << kColorLabel << "  eta(s): " << kColorData << eta
     << kColorLabel << "  rss(MiB): " << kColorData << (usage.ru_maxrss / 1024);

  // Reset color.
  ss << kColorReset << kClearEOL;

  // Reset cursor to left (again), so that any other stdout/stderr (spdlog)
  // Will start on a freshish line.
  ss << "\r";

  // Emit entire string in one atomic operation.
  std::cerr << ss.str();
}

void App::RunSerially() {
  try {
    RunProducer();
    RunConsumer();
    data_writer_.FlushActorIdMap();
    data_writer_.FlushNodeIdMap();
    data_writer_.FlushNodeQueue();
    map_block_writer_.FlushBlockQueue();
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

  spdlog::trace("Threads started.");

  // Ultra cheesy progress bar.
  while (map_block_queue_.idle_wait(kProgressInterval)) {
    DisplayProgress();
  }

  spdlog::trace("Main thread waiting for worker to finish.");

  producer_thread.join();
  for (auto &t : consumer_threads) {
    t.join();
  }

  spdlog::info("Flushing output data...");
  // TODO: Run the datawriter flushers in threads.
  data_writer_.FlushActorIdMap();
  data_writer_.FlushNodeIdMap();
  data_writer_.FlushNodeQueue();
  map_block_writer_.FlushBlockQueue();

  stats_.SetTombstone();
  stats_merge_thread.join();
}

// TODO: Read these from a text file.
void App::PreregisterContentIds() {
  node_ids_.Add("");       // 0 (b/c we don't allow null values).
  node_ids_.Add("ignore"); // 1
  node_ids_.Add("air");    // 2

  // We use "owner 0" to mean "no owner" (not null in database).
  actor_ids_.Add(""); // 0
}

void App::Run() {
  if (!config_.pattern_filename.empty()) {
    std::ifstream ifs(config_.pattern_filename);
    node_filter_.Load(ifs);
    spdlog::info("Loaded {0} patterns into the node name filter from {1}.",
                 node_filter_.size(), config_.pattern_filename);
  }

  PreregisterContentIds();

  const auto t0 = std::chrono::steady_clock::now();
  if (config_.threads) {
    RunThreaded();
  } else {
    RunSerially();
  }
  const auto t1 = std::chrono::steady_clock::now();

  const std::chrono::duration<double> diff = t1 - t0;
  const double rate = stats_.QueuedBlocks() / diff.count();
  spdlog::info("Processed {0} blocks in {1:.2f} seconds via {2} threads for "
               "{3:.2f} blocks/sec, {4:.2f} blocks/sec/thread.",
               stats_.QueuedBlocks(), diff.count(), config_.threads, rate,
               rate / config_.threads);
  spdlog::info("Unique nodes: {0}", node_ids_.size());
  spdlog::info("Unique actors: {0}", actor_ids_.size());
}
