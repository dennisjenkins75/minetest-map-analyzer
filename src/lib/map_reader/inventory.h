#ifndef _MT_MAP_SEARCH_INVENTORY_H_
#define _MT_MAP_SEARCH_INVENTORY_H_

#include <map>
#include <string>
#include <vector>

class BlobReader;
class Inventory;

// A single inventory, like "Main", "Src", "Fuel", etc...
// Just a list of item strings (w/ trailing "\n" stripped off).
class InventoryList {
public:
  InventoryList() : list_() {}

  uint64_t total_minegeld() const;

  void add(std::string &&item_str) { list_.push_back(std::move(item_str)); }

  void clear() { list_.clear(); }
  size_t size() const { return list_.size(); }
  bool empty() const { return list_.empty(); }

private:
  std::vector<std::string> list_;
};

// Many nodes can have multiple inventories.
// Maps inventory names to list of items for that specific name.
class Inventory {
public:
  Inventory() : lists_() {}

  // Returns 'true' if successful, false on error.
  bool deserialize_inventory(BlobReader &blob);

  uint64_t total_minegeld() const;

private:
  std::unordered_map<std::string, InventoryList> lists_;
};

#endif // _MT_MAP_SEARCH_INVENTORY_H_
