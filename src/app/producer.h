#ifndef _MT_MAP_PRODUCER_H
#define _MT_MAP_PRODUCER_H

#include <sqlite3.h>

#include "src/app/mapblock_queue.h"
#include "src/lib/map_reader/pos.h"

void Producer(MapBlockQueue *queue, sqlite3 *db, const MapBlockPos &min,
              const MapBlockPos &max);

#endif // _MT_MAP_PRODUCER_H
