#pragma once

#include "src/app/config.h"
#include "src/lib/database/db-map-interface.h"

std::unique_ptr<MapInterface> CreateMapInterface(const Config &config);
