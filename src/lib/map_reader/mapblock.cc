#include <algorithm>
#include <cassert>
#include <iostream>

#include "mapblock.h"
#include "node.h"
#include "utils.h"

MapBlock::MapBlock()
    : valid_(false), version_(0), flags_(0), lighting_complete_(0),
      timestamp_(0), num_name_id_mappings_(0), content_width_(0),
      params_width_(0), nodes_() {}

bool MapBlock::deserialize(BlobReader &blob, int64_t pos_id) {
  // u8 version
  if (!blob.read_u8(&version_, "version")) {
    return false;
  }

  // u8 flags
  if (!blob.read_u8(&flags_, "flags")) {
    return false;
  }

  // u16 lighting_complete
  if (!blob.read_u16(&lighting_complete_, "lighting_complete")) {
    return false;
  }

  if (version_ >= 29) {
    // u32 timestamp
    if (!blob.read_u32(&timestamp_, "timestamp")) {
      return false;
    }

    if (!deserialize_name_id_mapping(blob)) {
      return false;
    }
  }

  // u8 content_width
  if (!blob.read_u8(&content_width_, "content_width")) {
    return false;
  }
  assert(content_width_ == 2);

  // u8 params_width
  if (!blob.read_u8(&params_width_, "params_width")) {
    return false;
  }
  assert(params_width_ == 2);

  // node data.
  if (!deserialize_nodes(blob)) {
    return false;
  }

  // node metadata.
  if (!deserialize_metadata(blob, pos_id)) {
    return false;
  }

  if (!deserialize_static_objects(blob)) {
    return false;
  }

  if (version_ <= 28) {
    if (!blob.read_u32(&timestamp_, "timestamp")) {
      return false;
    }

    if (!deserialize_name_id_mapping(blob)) {
      return false;
    }
  }

  if (!deserialize_node_timers(blob)) {
    return false;
  }

  if (blob.remaining()) {
    std::cout << RED << "Left over data: " << blob.remaining() << CLEAR << "\n";

    if (DEBUG) {
      const size_t len = std::min(static_cast<size_t>(128), blob.remaining());
      std::cout << to_hex_block(blob.ptr(), len);
    }

    return false;
  }

  return true;
}

bool MapBlock::deserialize_nodes(BlobReader &blob) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_nodes:\n" << CLEAR;
  }

  // node data (zlib-compressed if version < 29).
  // We expect 16384 bytes.
  std::vector<uint8_t> node_buffer;
  if (!blob.decompress_zlib(&node_buffer, "nodes")) {
    return false;
  }
  if (node_buffer.size() != NODE_DATA_SIZE) {
    std::cout << "node_buffer.size() == " << node_buffer.size() << "\n";
    return false;
  }

  nodes_.resize(NODES_PER_BLOCK);

  for (size_t i = 0; i < NODES_PER_BLOCK; i++) {
    const uint16_t *p0 =
        reinterpret_cast<const uint16_t *>(node_buffer.data() + i * 2);
    nodes_[i].param_0 = ntohs(*p0);
    nodes_[i].param_1 = node_buffer[PARAM0_SIZE + i];
    nodes_[i].param_2 = node_buffer[PARAM0_SIZE + PARAM1_SIZE + i];
  }

  return true;
}

bool MapBlock::deserialize_metadata(BlobReader &blob, int64_t pos_id) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_metadata:\n" << CLEAR;
  }

  std::vector<uint8_t> meta_buffer;
  if (!blob.decompress_zlib(&meta_buffer, "metadata")) {
    return false;
  }
  BlobReader r(meta_buffer);

  if (DEBUG) {
    const size_t len = std::min(static_cast<size_t>(128), r.remaining());
    std::cout << to_hex_block(r.ptr(), len);
  }

  uint8_t version = 0;
  if (!r.read_u8(&version, "meta.version")) {
    return false;
  }
  if (!version) {
    return true;
  } // no metadata
  if (version != 2) {
    return false;
  } // expect version 2.

  uint16_t count = 0;
  if (!r.read_u16(&count, "meta.count")) {
    return false;
  }

  for (uint16_t meta_idx = 0; meta_idx < count; meta_idx++) {
    uint16_t local_pos = 0;
    if (!r.read_u16(&local_pos, "meta.pos")) {
      return false;
    }

    const NodePos pos(pos_id, local_pos);

    assert(local_pos < NODES_PER_BLOCK);
    nodes_.at(local_pos).deserialize_metadata(r, version, pos);
  }

  return true;
}

bool MapBlock::deserialize_name_id_mapping(BlobReader &blob) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_name_id_mapping:\n" << CLEAR;
    const size_t len = std::min(static_cast<size_t>(128), blob.remaining());
    std::cout << to_hex_block(blob.ptr(), len);
  }

  uint8_t nim_version = 0;
  if (!blob.read_u8(&nim_version, "nim.version")) {
    return false;
  }
  if (nim_version != 0) {
    return false;
  }

  uint16_t nim_count = 0;
  if (!blob.read_u16(&nim_count, "nim.count")) {
    return false;
  }

  name_id_mapping_.resize(nim_count, "");

  for (; nim_count > 0; --nim_count) {
    uint16_t id = 0;
    if (!blob.read_u16(&id, "nim.id")) {
      return false;
    }
    if (id >= 4096) {
      return false;
    }

    uint16_t name_len = 0;
    if (!blob.read_u16(&name_len, "nim.name_len")) {
      return false;
    }

    std::string name;
    if (!blob.read_str(&name, name_len, "nim.name")) {
      return false;
    }

    if (id > name_id_mapping_.size()) {
      name_id_mapping_.resize(id, "");
    }

    name_id_mapping_[id] = name;
  }

  return true;
}

// For now, we don't care about them, so just parse and toss them.
bool MapBlock::deserialize_static_objects(BlobReader &blob) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_static_objects:\n" << CLEAR;
    const size_t len = std::min(static_cast<size_t>(256), blob.remaining());
    std::cout << to_hex_block(blob.ptr(), len);
  }

  uint8_t obj_version = 0;
  if (!blob.read_u8(&obj_version, "static_object.version")) {
    return false;
  }
  if (obj_version != 0) {
    return false;
  }

  uint16_t obj_count = 0;
  if (!blob.read_u16(&obj_count, "static_object.count")) {
    return false;
  }

  for (; obj_count > 0; --obj_count) {
    uint8_t type = 0;
    if (!blob.read_u8(&type, "static_object.type")) {
      return false;
    }

    int32_t x, y, z;
    if (!blob.read_s32(&x, "static_object.x")) {
      return false;
    }
    if (!blob.read_s32(&y, "static_object.y")) {
      return false;
    }
    if (!blob.read_s32(&z, "static_object.z")) {
      return false;
    }

    uint16_t data_size = 0;
    if (!blob.read_u16(&data_size, "static_object.data_size")) {
      return false;
    }
    if (!blob.skip(data_size, "static_object.data")) {
      return false;
    }
  }

  return true;
}

bool MapBlock::deserialize_node_timers(BlobReader &blob) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_static_objects:\n" << CLEAR;
    const size_t len = std::min(static_cast<size_t>(256), blob.remaining());
    std::cout << to_hex_block(blob.ptr(), len);
  }

  uint8_t single_timer_len = 0;
  if (!blob.read_u8(&single_timer_len, "timer.len")) {
    return false;
  }

  uint16_t num_of_timers = 0;
  if (!blob.read_u16(&num_of_timers, "timer.count")) {
    return false;
  }

  for (; num_of_timers > 0; --num_of_timers) {
    uint16_t pos = 0;
    if (!blob.read_u16(&pos, "timer.pos")) {
      return false;
    }

    int32_t timeout = 0;
    if (!blob.read_s32(&timeout, "timer.timeout")) {
      return false;
    }

    int32_t elapsed = 0;
    if (!blob.read_s32(&elapsed, "timer.elapsed")) {
      return false;
    }
  }

  return true;
}
