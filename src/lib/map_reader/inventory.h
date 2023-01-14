#pragma once

#include <unordered_map>
#include <string>
#include <vector>

class BlobReader;
class Inventory;

// A single inventory, like "Main", "Src", "Fuel", etc...
// Just a list of item strings (w/ trailing "\n" stripped off).
class InventoryList {
public:
  InventoryList() : items_() {}

  uint64_t total_minegeld() const;

  void add(std::string &&item_str) { items_.push_back(std::move(item_str)); }

  void clear() { items_.clear(); }
  size_t size() const { return items_.size(); }
  bool empty() const { return items_.empty(); }

  const std::vector<std::string> &items() const { return items_; }

private:
  std::vector<std::string> items_;
};

// Many nodes can have multiple inventories.
// Maps inventory names to list of items for that specific name.
class Inventory {
public:
  Inventory() : lists_() {}

  // Returns 'true' if successful, false on error.
  bool deserialize_inventory(BlobReader &blob);

  uint64_t total_minegeld() const;

  bool empty() const { return lists_.empty(); }

  const std::unordered_map<std::string, InventoryList> &lists() const {
    return lists_;
  }

private:
  std::unordered_map<std::string, InventoryList> lists_;
};
