#ifndef _MT_MAP_SEARCH_INVENTORY_H_
#define _MT_MAP_SEARCH_INVENTORY_H_

#include <map>
#include <string>
#include <vector>

// A single inventory, like "Main", "Src", "Fuel", etc...
// Just a list of item strings (w/ trailing "\n" stripped off).
using InventoryList = std::vector<std::string>;

// Many nodes can have multiple inventories.
// Maps inventory names to list of items for that specific name.
using Inventory = std::map<std::string, InventoryList>;

// Returns 'true' if successful, false on error.
class BlobReader;
bool deserialize_inventory(Inventory *dest, BlobReader &blob);

uint64_t total_minegeld_in_inventory(const Inventory &inventory);

#endif  // _MT_MAP_SEARCH_INVENTORY_H_
