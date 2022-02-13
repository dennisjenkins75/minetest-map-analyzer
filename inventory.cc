#include <iostream>
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

bool deserialize_inventory(Inventory *dest, BlobReader &blob) {
  if (DEBUG) {
    std::cout << CYAN << "deserialize_inventory:\n" << CLEAR;
    const size_t len = std::min(static_cast<size_t>(1024), blob.remaining());
    std::cout << to_hex_block(blob.ptr(), len);
  }

  // Current list that we're building.
  InventoryList current;
  std::string list_name;

  std::string line;
  while (blob.read_line(&line, "inventory")) {
    if (DEBUG) {
      std::cout << GREEN << line << RED << "." << CLEAR << "\n";
    }

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
      current.push_back(line.substr(5));
      continue;
    }

    if (std::regex_match(line, sm, re_item)) {
      if (list_name.empty()) { return false; }
      current.push_back(sm[1]);
      continue;
    }

    if (std::regex_match(line, sm, re_empty)) {
      if (list_name.empty()) { return false; }
      current.push_back(sm[1]);
      continue;
    }

    if (line == end_list) {
      (*dest)[list_name] = std::move(current);
      list_name.clear();
      current.clear();
      continue;
    }

    if (line == end_inventory) {
      return true;
    }

    // Junk data?
    return false;
  }

  return false;
}

// "currency:minegeld_10 46"


static const std::regex re_minegeld("currency:minegeld(_)?(\\d+)?( \\d+)?");

uint64_t total_minegeld_in_inventory(const Inventory &inventory) {
  uint64_t total = 0;

  for (const auto &type: inventory) {
    for (const auto &item_str: type.second) {
      std::smatch sm;
      if (std::regex_match(item_str, sm, re_minegeld)) {
        if (sm.size() == 4) {
          int denom = strtoul(sm[2].str().c_str(), nullptr, 10);
          int qty = strtoul(sm[3].str().c_str(), nullptr, 10);
          total += (denom * qty);
          continue;
        }

/*
        std::cout << "sm.size(): " << sm.size() << "\n";
        //for (const auto &foo: sm) { std::cout << "  " << foo << "\n"; }
        std::cout << RED << item_str << CLEAR << "\n";
        exit(-1);
*/
      }

      if (item_str.find("currency:minegeld_cent") != std::string::npos) {
        continue;
      }
      if (item_str.find("currency:minegeld_bundle") != std::string::npos) {
        continue;
      }

      if (item_str.find("currency:minegeld") != std::string::npos) {
        std::cout << RED << item_str << CLEAR << "\n";
        exit(-1);
      }
    }
  }

  return total;
}
