#pragma once

#include <pqxx/pqxx>

#include "src/lib/database/db-map-interface.h"

class MapInterfacePostgresql : public MapInterface {
public:
  MapInterfacePostgresql() = delete;
  explicit MapInterfacePostgresql(const std::string &connection_str);
  virtual ~MapInterfacePostgresql() {}

  std::optional<Blob> LoadMapBlock(const MapBlockPos &pos) override;

  bool ProduceMapBlocks(
      const MapBlockPos &min, const MapBlockPos &max,
      std::function<bool(const MapBlockPos &, int64_t)> callback) override;

  void DeleteMapBlocks(const std::vector<MapBlockPos> &list) override;

protected:
  std::string pqxx_url_;
  std::unique_ptr<pqxx::connection> connection_;
};
