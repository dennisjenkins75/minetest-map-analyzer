#ifndef _MT_MAP_SEARCH_NODE_H_
#define _MT_MAP_SEARCH_NODE_H_

#include <vector>

#include "inventory.h"
#include "pos.h"

class BlobReader;
class MapBlock;

class Node {
friend class MapBlock;
 public:
  struct MetaDataVar {
    std::string key;
    std::string value;
    uint8_t private_;
  };

  Node():
    param_0(0),
    param_1(0),
    param_2(0),
    metadata_(),
    inventory_()
  {}


  uint16_t param0() const { return param_0; }
  uint8_t param1() const { return param_1; }
  uint8_t param2() const { return param_2; }

  const std::vector<MetaDataVar>& metadata() const { return metadata_; }
  const Inventory& inventory() const { return inventory_; }
  std::string get_meta(const std::string &key) const;

 private:
  uint16_t param_0;
  uint8_t param_1;
  uint8_t param_2;

  std::vector<MetaDataVar> metadata_;

  Inventory inventory_;

  // Extracts metadata from input stream for THIS NODE ONLY.
  // Called immediately after MapBlock::deserialize_metadata() extracts the
  // per-node `pos`.
  bool deserialize_metadata(BlobReader &blob, uint8_t version, const Pos &pos);
};


#endif // _MT_MAP_SEARCH_NODE_H_
