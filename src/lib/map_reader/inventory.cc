#include <regex>

#include "blob_reader.h"
#include "inventory.h"
#include "utils.h"

// https://github.com/minetest/minetest/blob/master/doc/world_format.txt#L554
// Read "\n" terminates strings until we find "EndInventory\n".

const std::regex re_new_list("List (\\w+) (\\d+)");
const std::regex re_width("Width (\\d+)");
const std::regex re_item("Item (.*)");
const std::regex re_empty("Empty");
const std::string end_list = "EndInventoryList";
const std::string end_inventory = "EndInventory";

void Inventory::deserialize_inventory(BlobReader &blob) {
  // Current list that we're building.
  InventoryList current;
  std::string list_name;

  while (true) {
    const std::string line = blob.read_line("inventory");

    std::smatch sm;
    if (std::regex_match(line, sm, re_new_list)) {
      list_name = sm[1];
      current.clear();
      continue;
    }

    if (std::regex_match(line, sm, re_width)) {
      // meh, we don't care about the width right now.
      continue;
    }

    // If the item string is HUGE then std::regex can exhaust its stack
    // and segfault.  Ex: A crated digtron inside a chest.
    if ((line.size() > 4096) && (line.substr(0, 5) == "Item ")) {
      current.add(std::move(line.substr(5)));
      continue;
    }

    if (std::regex_match(line, sm, re_item)) {
      if (list_name.empty()) {
        // `list_name` should eb filled in BEFORE we find items.
        throw SerializationError(blob, "inventory", "list_name.empty()");
      }
      current.add(std::move(sm[1]));
      continue;
    }

    if (std::regex_match(line, sm, re_empty)) {
      if (list_name.empty()) {
        // `list_name` should eb filled in BEFORE we find items.
        throw SerializationError(blob, "inventory", "list_name.empty()");
      }
      current.add(std::move(sm[1]));
      continue;
    }

    if (line == end_list) {
      lists_[list_name] = std::move(current);
      list_name.clear();
      current.clear();
      continue;
    }

    if (line == end_inventory) {
      break;
    }

    throw SerializationError(blob, "inventory", "Junk string? " + line);
  }
}

// "currency:minegeld_10 46"

static const std::regex re_minegeld("currency:minegeld(_)?(\\d+)?( \\d+)?");

uint64_t InventoryList::total_minegeld() const {
  uint64_t total = 0;

  for (const auto &item_str : items_) {
    std::smatch sm;
    if (std::regex_match(item_str, sm, re_minegeld)) {
      if (sm.size() == 4) {
        int denom = strtoul(sm[2].str().c_str(), nullptr, 10);
        int qty = strtoul(sm[3].str().c_str(), nullptr, 10);
        total += (denom * qty);
        continue;
      }
    }

    if (item_str.find("currency:minegeld_cent") != std::string::npos) {
      continue;
    }
    if (item_str.find("currency:minegeld_bundle") != std::string::npos) {
      continue;
    }

    if (item_str.find("currency:minegeld") != std::string::npos) {
      // TODO: Not sure what to do here.
      // Maybe the current mod added a new node sub-type?
      continue;
    }
  }

  return total;
}

uint64_t Inventory::total_minegeld() const {
  uint64_t total = 0;

  for (const auto &type : lists_) {
    total += type.second.total_minegeld();
  }

  return total;
}
