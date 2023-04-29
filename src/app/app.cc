#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "src/app/app.h"
#include "src/lib/util/memory_stats.h"

static constexpr size_t kMegabyte = 1024 * 1024;
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

  const MemoryStats ms = GetMemoryStats();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> time_diff = now - start_time_;

  // Avoid divide by zero.
  if (!time_diff.count() || !stats_.queued_map_blocks) {
    return;
  }

  const uint64_t sofar = stats_.good_map_blocks + stats_.bad_map_blocks;
  const uint64_t remaining = stats_.queued_map_blocks - sofar;
  const double perc =
      100.0 * static_cast<double>(sofar) / stats_.queued_map_blocks;
  const double blocks_per_second = sofar / time_diff.count();
  const double eta = remaining / blocks_per_second;
  const size_t matrix = block_data_.size();

  std::stringstream ss;

  // Return cursor to left side, same line.
  ss << kCursorLeft;

  // Colors!
  ss << kColorData << sofar << kColorLabel << " of " << kColorData
     << stats_.queued_map_blocks << kColorLabel << " (" << std::fixed
     << std::setprecision(3) << kColorData << perc << kColorLabel
     << "%) b/s: " << std::setprecision(1) << kColorData << blocks_per_second
     << kColorLabel << " eta: " << kColorData << eta << kColorLabel
     << " vsz: " << kColorData << (ms.vsize / kMegabyte);

  ss << kColorLabel << " # " << kColorData << matrix << " " << kColorLabel;

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
    preserve_queue_.SetTombstone();
    preserve_queue_.MergeThread();
    ApplyPreserveFlags();
    map_block_writer_.FlushBlockQueue();
  } catch (const Sqlite3Error &err) {
    spdlog::error("Sqlite3Error: {0}", err.what());
  }
}

void App::RunThreaded() {
  std::thread producer_thread(&App::RunProducer, this);

  std::thread preserve_thread(&PreserveQueue::MergeThread, &preserve_queue_);

  std::vector<std::thread> consumer_threads;
  consumer_threads.reserve(config_.threads);
  for (int i = 0; i < config_.threads; ++i) {
    consumer_threads.push_back(std::thread(&App::RunConsumer, this));
  }

  spdlog::trace("Threads started.");

  // Ultra cheesy progress bar.
  while (map_block_queue_.idle_wait(kProgressInterval)) {
    DisplayProgress();
    stats_.SetPeakVSize(GetMemoryStats().vsize);
  }

  spdlog::trace("Main thread waiting for worker to finish.");

  producer_thread.join();
  for (auto &t : consumer_threads) {
    t.join();
  }

  spdlog::info("preserve_queue_.SetTombstone()");
  preserve_queue_.SetTombstone();

  stats_.flush_time = std::chrono::steady_clock::now();
  spdlog::info("Flushing output data...");
  // TODO: Run the datawriter flushers in threads.
  data_writer_.FlushActorIdMap();
  data_writer_.FlushNodeIdMap();
  data_writer_.FlushNodeQueue();

  stats_.SetPeakVSize(GetMemoryStats().vsize);
  spdlog::info("preserve_thread.join()");
  preserve_thread.join();
  ApplyPreserveFlags();

  spdlog::info("map_block_writer_.FlushBlockQueue()");
  stats_.SetPeakVSize(GetMemoryStats().vsize);
  map_block_writer_.FlushBlockQueue();

  stats_.SetPeakVSize(GetMemoryStats().vsize);
  spdlog::info("Peak RAM usage: {0} MiB", stats_.peak_vsize_bytes / kMegabyte);
}

// TODO: Read these from a text file.
void App::PreregisterContentIds() {
  node_ids_.Add("");       // 0 (b/c we don't allow null values).
  node_ids_.Add("ignore"); // 1
  node_ids_.Add("air");    // 2

  // We use "owner 0" to mean "no owner" (not null in database).
  actor_ids_.Add(""); // 0
}

void App::ApplyPreserveFlags() {
  PreserveQueue::MapBlockPosSet queue = preserve_queue_.SurrenderFinalSet();
  spdlog::trace("App::ApplyPreserveFlags() enter {0} items", queue.size());

  spdlog::info("PreserveQueue final set: {0}", queue.size());
  for (const MapBlockPos &pos : queue) {
    block_data_.Ref(pos).preserve = true;
  }

  spdlog::trace("App::ApplyPreserveFlags() exit");
}

void App::Run() {
  if (!config_.pattern_filename.empty()) {
    std::ifstream ifs(config_.pattern_filename);
    node_filter_.Load(ifs);
    spdlog::info("Loaded {0} patterns into the node name filter from {1}.",
                 node_filter_.size(), config_.pattern_filename);
  }

  PreregisterContentIds();

  stats_.start_time = std::chrono::steady_clock::now();
  if (config_.threads) {
    RunThreaded();
  } else {
    RunSerially();
  }
  stats_.end_time = std::chrono::steady_clock::now();

  if (!config_.stats_filename.empty()) {
    WriteStatsFile(config_.stats_filename);
  }

  const std::chrono::duration<double> diff =
      stats_.end_time - stats_.start_time;
  const double rate = stats_.queued_map_blocks / diff.count();
  spdlog::info("Processed {0} blocks in {1:.2f} seconds via {2} threads for "
               "{3:.2f} blocks/sec, {4:.2f} blocks/sec/thread.",
               stats_.queued_map_blocks, diff.count(), config_.threads, rate,
               rate / config_.threads);
  spdlog::info("Unique nodes: {0}", node_ids_.size());
  spdlog::info("Unique actors: {0}", actor_ids_.size());
}

void App::WriteStatsFile(const std::string filename) {
  std::ofstream ofs(filename, std::ios::in | std::ios::app | std::ios::out);
  if (ofs.is_open()) {
    const std::chrono::duration<double> r0 =
        stats_.flush_time - stats_.start_time;
    const std::chrono::duration<double> r1 =
        stats_.end_time - stats_.flush_time;

    ofs << config_.threads << "," << stats_.queued_map_blocks << ","
        << std::fixed << std::setprecision(2) << r0.count() << ","
        << std::setprecision(2) << r1.count() << "," << stats_.peak_vsize_bytes
        << "\n";

    spdlog::info("Runtime stats appended to {0}", filename);
  } else {
    spdlog::error("Failed to open {0} for append\n", filename);
  }
}
