// mapblock.h

// https://github.com/minetest/minetest/blob/master/doc/world_format.txt

#pragma once

#include <string>
#include <vector>

#include "src/lib/id_map/id_map.h"
#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/node.h"

class MapBlock {
public:
  static constexpr size_t MAP_BLOCKSIZE = 16;
  static constexpr size_t NODES_PER_BLOCK =
      MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
  static constexpr size_t PARAM0_SIZE = sizeof(uint16_t) * NODES_PER_BLOCK;
  static constexpr size_t PARAM1_SIZE = sizeof(uint8_t) * NODES_PER_BLOCK;
  static constexpr size_t PARAM2_SIZE = sizeof(uint8_t) * NODES_PER_BLOCK;
  static constexpr size_t NODE_DATA_SIZE =
      PARAM0_SIZE + PARAM1_SIZE + PARAM2_SIZE;
  static_assert(NODE_DATA_SIZE == 16384);

  MapBlock();

  // TODO: Change 2nd arg to 'const MapBlockPos &pos'.
  void deserialize(BlobReader &blob, int64_t pos_id, ThreadLocalIdMap &id_map);

  uint8_t version() { return version_; }

  const std::vector<Node> &nodes() const { return nodes_; }

  size_t unique_content_ids() const { return param0_map_.size(); }

private:
  // Overall, was the deserialiation 100% successful?
  bool valid_;

  // Exact fields (see world_format.txt spec);
  uint8_t version_;
  uint8_t flags_;
  uint16_t lighting_complete_;
  uint32_t timestamp_;
  uint16_t num_name_id_mappings_;
  uint8_t content_width_;
  uint8_t params_width_;

  // Has 4096 entries.
  // Index = p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X
  std::vector<Node> nodes_;

  // Maps param0 to global node_id (from ThreadLocalIdMap).
  // Index is param0, Value is global node id.
  std::vector<int> param0_map_;

  // Format #28: Minetest-5.4.x, Multicraft-2.x
  void deserialize_format_28(BlobReader &blob, int64_t pos_id,
                             ThreadLocalIdMap &id_map);

  // Format #29: Minetest-5.5.x.
  void deserialize_format_29(BlobReader &blob_zstd, int64_t pos_id,
                             ThreadLocalIdMap &id_map);

  void deserialize_nodes_28(BlobReader &blob);
  void deserialize_nodes_29(BlobReader &blob);

  // TODO: Change 2nd arg to 'const MapBlockPos &pos'.
  void deserialize_metadata_28(BlobReader &blob, int64_t pos_id);
  void deserialize_metadata_29(BlobReader &blob, int64_t pos_id);

  void deserialize_name_id_mapping(BlobReader &blob, ThreadLocalIdMap &id_map);

  void deserialize_static_objects(BlobReader &blob);

  void deserialize_node_timers(BlobReader &blob);

  void remap_param0();

  void verify_all_data_consumed(BlobReader &blob);
};
