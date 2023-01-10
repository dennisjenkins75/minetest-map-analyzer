#include "src/app/app.h"
#include "src/app/schema/schema.h"

App::App(Config &config)
    : config_(config), actor_ids_(), node_ids_(), map_block_queue_() {}

void App::Run() {
  VerifySchema(config_.data_filename);
  RunProducer();
  RunConsumer();
}
