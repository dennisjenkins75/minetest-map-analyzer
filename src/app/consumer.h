#pragma once

#include "src/app/config.h"
#include "src/app/mapblock_queue.h"

void RunConsumer(const Config &config, MapBlockQueue *queue);
