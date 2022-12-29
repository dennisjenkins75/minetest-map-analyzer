#include "src/lib/database/db-map-interface.h"
#include "src/lib/database/db-map-sqlite3.h"

std::unique_ptr<MapInterface>
MapInterface::Create(std::string_view driver_name,
                     std::string_view connection_str) {
  if (driver_name == "sqlite3") {
    return std::make_unique<MapInterfaceSqlite3>(connection_str);
  }

  std::string err("Invalid database driver name: ");
  err += driver_name;
  throw DatabaseError(err);
}
