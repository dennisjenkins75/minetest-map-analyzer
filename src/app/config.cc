#include <spdlog/spdlog.h>
#include <thread>

#include "src/app/config.h"

// Default count of mapblocks the producer accumulates before passing them
// to the consumer.
static constexpr size_t kDefaultProducerBatchSize = 2048;

// Default mapblock radius to preserve adjacent anthropocene blocks.
// The "mapblock removal" code will preserve (not delete) any mapblock within
// this mapblock distance from any mapblock considered "anthropocene".
static constexpr int kDefaultPreserveRadius = 5;

// Max size of per-thread "preserve set" of mapblocks, before flushing to
// common queue.
static constexpr size_t kDefaultAnthropoceneFlushThreshold = 2048;

// Max size of common preserve queue before flushing to global block hashmap.
// Locking of this hashmap is expensive, hence the desire to reduce accesses
// to it.
static constexpr size_t kDefaultPreserveLimit = 32768;

Config::Config()
    : min_pos(MapBlockPos::min()), max_pos(MapBlockPos::max()),
      driver_type(MapDriverType::SQLITE), map_filename(), out_filename(),
      pattern_filename(), stats_filename(), threads(0),
      max_load_avg(std::thread::hardware_concurrency()),
      preserve_radius(kDefaultPreserveRadius),
      producer_batch_size(kDefaultProducerBatchSize),
      anthropocene_flush_threshold(kDefaultAnthropoceneFlushThreshold),
      preserve_limit(kDefaultPreserveLimit), track_minegeld(false) {}

void DebugLogConfig(const Config &config) {
  spdlog::debug("config.map_filename: {0}", config.map_filename);
  spdlog::debug("config.out_filename: {0}", config.out_filename);
  spdlog::debug("config.pattern_filename: {0}", config.pattern_filename);
  spdlog::debug("config.stats_filename: {0}", config.stats_filename);
  spdlog::debug("config.min_pos: {0} {1}", config.min_pos.str(),
                config.min_pos.MapBlockId());
  spdlog::debug("config.max_pos: {0} {1}", config.max_pos.str(),
                config.max_pos.MapBlockId());
  spdlog::debug("config.preserve_radius: {0}", config.preserve_radius);
  spdlog::debug("config.threads: {0}", config.threads);
  spdlog::debug("config.max_load_avg: {0}", config.max_load_avg);
  spdlog::debug("config.producer_batch_size: {0}", config.producer_batch_size);
  spdlog::debug("config.anthropocene_flush_threshold: {0}",
                config.anthropocene_flush_threshold);
  spdlog::debug("config.preserve_limit: {0}", config.preserve_limit);
  spdlog::debug("config.track_minegeld: {0}", config.track_minegeld);
}
