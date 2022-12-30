#include "src/app/factory.h"

std::unique_ptr<MapInterface> CreateMapInterface(const Config &config) {
  // TODO: Add support for "postgresql" connections as well.
  return MapInterface::Create("sqlite3", config.map_filename);
}
