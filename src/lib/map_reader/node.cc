#include <algorithm>
#include <cassert>
#include <iostream>

#include "src/lib/map_reader/blob_reader.h"
#include "src/lib/map_reader/node.h"
#include "src/lib/map_reader/utils.h"

bool Node::deserialize_metadata(BlobReader &blob, uint8_t version,
                                const NodePos &pos) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_metadata (node) " << pos.str() << "\n";
  }

  uint32_t num_vars = 0;
  if (!blob.read_u32(&num_vars, "meta.num_vars")) {
    return false;
  }

  for (uint32_t v = 0; v < num_vars; v++) {
    MetaDataVar var;

    uint16_t key_len = 0;
    if (!blob.read_u16(&key_len, "meta.key_len")) {
      return false;
    }
    if (!blob.read_str(&var.key, key_len, "meta.key")) {
      return false;
    }
    if (DEBUG) {
      std::cout << "meta.key: " << GREEN << var.key << CLEAR << "\n";
    }

    uint32_t val_len = 0;
    if (!blob.read_u32(&val_len, "meta.val_len")) {
      return false;
    }
    if (!blob.read_str(&var.value, val_len, "meta.val")) {
      return false;
    }
    if (DEBUG) {
      std::cout << "meta.value: " << GREEN << var.value << CLEAR << "\n";
    }

    if (version >= 2) {
      if (!blob.read_u8(&var.private_, "meta.private")) {
        return false;
      }
      if (var.private_ > 1) {
        std::cout << "Unexpected value for 'meta.private': "
                  << to_hex(&var.private_, 1) << "\n";
        return false;
      }
    }

    if (DEBUG) {
      std::cout << GREEN << v << ": " << var.key << " = " << var.value << CLEAR
                << "\n";
    }

    metadata_.push_back(std::move(var));
  }

  if (!deserialize_inventory(&inventory_, blob)) {
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
