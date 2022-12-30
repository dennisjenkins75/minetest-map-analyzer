#include <spdlog/spdlog.h>

#include "src/app/config.h"

void DebugLogConfig(const Config &config) {
  spdlog::debug("config.min_pos: {0} {1}", config.min_pos.str(),
                config.min_pos.MapBlockId());
  spdlog::debug("config.max_pos: {0} {1}", config.max_pos.str(),
                config.max_pos.MapBlockId());
  spdlog::debug("config.threads: {0}", config.threads);
  spdlog::debug("config.max_load_avg: {0}", config.max_load_avg);
}
