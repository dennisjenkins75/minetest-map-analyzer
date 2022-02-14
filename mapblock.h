// mapblock.h

// https://github.com/minetest/minetest/blob/master/doc/world_format.txt

#ifndef _MT_MAP_SEARCH_MAPBLOCK_H_
#define _MT_MAP_SEARCH_MAPBLOCK_H_

#include <vector>
#include <string>

#include "blob_reader.h"
class Node;

class MapBlock {
 public:
  static constexpr size_t MAP_BLOCKSIZE = 16;
  static constexpr size_t NODES_PER_BLOCK = MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
  static constexpr size_t PARAM0_SIZE = sizeof(uint16_t) * NODES_PER_BLOCK;
  static constexpr size_t PARAM1_SIZE = sizeof(uint8_t) * NODES_PER_BLOCK;
  static constexpr size_t PARAM2_SIZE = sizeof(uint8_t) * NODES_PER_BLOCK;
  static constexpr size_t NODE_DATA_SIZE =
      PARAM0_SIZE + PARAM1_SIZE + PARAM2_SIZE;
  static_assert(NODE_DATA_SIZE == 16384);

  MapBlock();

  bool deserialize(BlobReader &blob, int64_t pos_id);

  uint8_t version() { return version_; }

  const std::vector<Node>& nodes() const { return nodes_; }

  const std::string& name_for_id(uint16_t id) const;

 private:
  // Overall, was the deserialiation 100% successfull?
  bool       valid_;

  // Exact fields (see world_format.txt spec);
  uint8_t    version_;
  uint8_t    flags_;
  uint16_t   lighting_complete_;
  uint32_t   timestamp_;
  uint16_t   num_name_id_mappings_;
  uint8_t    content_width_;
  uint8_t    params_width_;

  // Has 4096 entries.
  // Index = p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X
  std::vector<Node> nodes_;

  // Maps IDs (param0) to names.
  std::vector<std::string> name_id_mapping_;

  bool deserialize_nodes(BlobReader &blob);

  // Extracts ALL metadata.
  bool deserialize_metadata(BlobReader &blob, int64_t pos_id);

  bool deserialize_name_id_mapping(BlobReader &blob);

  bool deserialize_static_objects(BlobReader &blob);

  bool deserialize_node_timers(BlobReader &blob);
};

#endif // _MT_MAP_SEARCH_MAPBLOCK_H_
