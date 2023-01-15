#include <algorithm>
#include <sstream>

#include "mapblock.h"
#include "node.h"
#include "utils.h"

MapBlock::MapBlock()
    : valid_(false), version_(0), flags_(0), lighting_complete_(0),
      timestamp_(0), num_name_id_mappings_(0), content_width_(0),
      params_width_(0), nodes_(), param0_map_() {}

void MapBlock::deserialize(BlobReader &blob, int64_t pos_id,
                           ThreadLocalIdMap &id_map) {
  version_ = blob.read_u8("version");
  if (version_ != 28) {
    throw SerializationError(blob, "MapBlock::deserialize",
                             std::string("Unsupported version ") +
                                 std::to_string(version_));
  }

  flags_ = blob.read_u8("flags");
  lighting_complete_ = blob.read_u16("lighting_complete");

  if (version_ >= 29) {
    timestamp_ = blob.read_u32("timestamp");
    deserialize_name_id_mapping(blob, id_map);
  }

  content_width_ = blob.read_u8("content_width");
  if (content_width_ != 2) {
    throw SerializationError(blob, "MapBlock::deserialize",
                             std::string("Unsupported content_width ") +
                                 std::to_string(content_width_));
  }

  params_width_ = blob.read_u8("params_width");
  if (params_width_ != 2) {
    throw SerializationError(blob, "MapBlock::deserialize",
                             std::string("Unsupported params_width ") +
                                 std::to_string(params_width_));
  }

  deserialize_nodes(blob);
  deserialize_metadata(blob, pos_id);
  deserialize_static_objects(blob);

  if (version_ <= 28) {
    timestamp_ = blob.read_u32("timestamp");
    deserialize_name_id_mapping(blob, id_map);
  }

  deserialize_node_timers(blob);
  remap_param0();

  if (blob.remaining()) {
    std::stringstream ss;
    const size_t len = std::min(static_cast<size_t>(128), blob.remaining());
    ss << "Left over data after deserialization.  Remaining byte sample: "
       << to_hex(blob.ptr(), len);
    throw SerializationError(blob, "MapBlock::deserialize", ss.str());
  }
}

void MapBlock::deserialize_nodes(BlobReader &blob) {
  // node data (zlib-compressed if version < 29).
  // We expect 16384 bytes.
  const std::vector<uint8_t> node_buffer = blob.decompress_zlib("nodes");

  if (node_buffer.size() != NODE_DATA_SIZE) {
    std::stringstream ss;
    ss << "Decompressed into " << node_buffer.size() << " nodes; expected "
       << NODE_DATA_SIZE << " instead.";
    throw SerializationError(blob, "MapBlock::deserialize_nodes", ss.str());
  }

  nodes_.resize(NODES_PER_BLOCK);

  for (size_t i = 0; i < NODES_PER_BLOCK; i++) {
    const uint16_t *p0 =
        reinterpret_cast<const uint16_t *>(node_buffer.data() + i * 2);
    nodes_[i].param_0 = ntohs(*p0);
    nodes_[i].param_1 = node_buffer[PARAM0_SIZE + i];
    nodes_[i].param_2 = node_buffer[PARAM0_SIZE + PARAM1_SIZE + i];
  }
}

void MapBlock::deserialize_metadata(BlobReader &blob, int64_t pos_id) {
  const std::vector<uint8_t> meta_buffer = blob.decompress_zlib("metadata");
  BlobReader r(meta_buffer);

  const uint8_t version = r.read_u8("meta.version");
  if (!version) {
    // No metadata to deserialize.
    return;
  }

  if (version != 2) {
    throw SerializationError(blob, "deserialize_metadata",
                             std::string("Unsupported meta.version value ") +
                                 std::to_string(version));
  }

  const uint16_t count = r.read_u16("meta.count");

  for (uint16_t meta_idx = 0; meta_idx < count; meta_idx++) {
    const uint16_t local_pos = r.read_u16("meta.pos");

    if (local_pos >= NODES_PER_BLOCK) {
      throw SerializationError(blob, "MapBlock::deserialize_metadata",
                               std::string("Invalid metadata.pos ") +
                                   std::to_string(local_pos));
    }

    const NodePos pos(pos_id, local_pos);

    nodes_.at(local_pos).deserialize_metadata(r, version, pos);
  }
}

void MapBlock::deserialize_name_id_mapping(BlobReader &blob,
                                           ThreadLocalIdMap &id_map) {
  const uint8_t nim_version = blob.read_u8("nim.version");
  if (nim_version != 0) {
    throw SerializationError(blob, "deserialize_name_id_mapping",
                             std::string("Unsupported nim.version value ") +
                                 std::to_string(nim_version));
  }

  uint16_t nim_count = blob.read_u16("nim.count");

  param0_map_.resize(nim_count, -1);

  for (; nim_count > 0; --nim_count) {
    const uint16_t id = blob.read_u16("nim.id");
    if (id >= 4096) {
      throw SerializationError(blob, "deserialize_name_id_mapping",
                               std::string("Illegal nim.id value ") +
                                   std::to_string(id));
    }

    const uint16_t name_len = blob.read_u16("nim.name_len");

    const std::string name = blob.read_str(name_len, "nim.name");

    if (id > param0_map_.size()) {
      param0_map_.resize(id, -1);
    }

    param0_map_[id] = id_map.Add(name);
  }
}

// For now, we don't care about them, so just parse and toss them.
void MapBlock::deserialize_static_objects(BlobReader &blob) {
  const uint8_t obj_version = blob.read_u8("static_object.version");
  if (obj_version != 0) {
    throw SerializationError(
        blob, "deserialize_static_objects",
        std::string("Unsupported static_object.version value ") +
            std::to_string(obj_version));
  }

  uint16_t obj_count = blob.read_u16("static_object.count");

  for (; obj_count > 0; --obj_count) {
    const uint8_t type __attribute__((unused)) =
        blob.read_u8("static_object.type");

    const int32_t x __attribute__((unused)) = blob.read_s32("static_object.x");
    const int32_t y __attribute__((unused)) = blob.read_s32("static_object.y");
    const int32_t z __attribute__((unused)) = blob.read_s32("static_object.z");
    const uint16_t data_size = blob.read_u16("static_object.data_size");

    // TODO: Actually import static objects.
    blob.skip(data_size, "static_object.data");
  }
}

void MapBlock::deserialize_node_timers(BlobReader &blob) {
  const uint8_t single_timer_len __attribute__((unused)) =
      blob.read_u8("timer.len");
  uint16_t num_of_timers = blob.read_u16("timer.count");

  for (; num_of_timers > 0; --num_of_timers) {
    const uint16_t pos __attribute__((unused)) = blob.read_u16("timer.pos");
    const int32_t timeout __attribute__((unused)) =
        blob.read_s32("timer.timeout");
    const int32_t elapsed __attribute__((unused)) =
        blob.read_s32("timer.elapsed");

    // TODO: Do something with the node timers.
  }
}

void MapBlock::remap_param0() {
  for (Node &node : nodes_) {
    node.param_0 = param0_map_.at(node.param_0);
  }
}
