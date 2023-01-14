#include <algorithm>
#include <cassert>
#include <iostream>

#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/utils.h"

bool Node::deserialize_metadata(BlobReader &blob, uint8_t version,
                                const NodePos &pos) {
  uint32_t num_vars = blob.read_u32("meta.num_vars");

  for (uint32_t v = 0; v < num_vars; v++) {
    MetaDataVar var;

    const uint16_t key_len = blob.read_u16("meta.key_len");
    var.key = blob.read_str(key_len, "meta.key");

    const uint32_t val_len = blob.read_u32("meta.val_len");
    var.value = blob.read_str(val_len, "meta.val");

    if (version >= 2) {
      var.private_ = blob.read_u8("meta.private");
      if (var.private_ > 1) {
        std::stringstream ss;
        ss << "Unexpected value for 'meta.private': "
           << to_hex(&var.private_, 1);
        throw SerializationError(ss.str());
      }
    }

    metadata_.push_back(std::move(var));
  }

  if (!inventory_.deserialize_inventory(blob)) {
    return false;
  }

  return true;
}

std::string Node::get_meta(const std::string &key) const {
  // This is a linear search, but we rarely need to search meta,
  // its usually small, and making is an unordered_map slows down the
  // program.

  for (const auto &meta : metadata_) {
    if (meta.key == key) {
      return meta.value;
    }
  }

  return "";
}

std::string Node::get_owner() const {
  for (const auto &meta : metadata_) {
    if ((meta.key == "owner") || (meta.key == "_owner")) {
      return meta.value;
    }
  }

  return "";
}
