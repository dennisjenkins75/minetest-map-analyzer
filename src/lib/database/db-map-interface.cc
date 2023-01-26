#include "src/lib/database/db-map-interface.h"
#include "src/lib/database/db-map-postgresql.h"
#include "src/lib/database/db-map-sqlite3.h"

std::unique_ptr<MapInterface>
MapInterface::Create(MapDriverType type, const std::string &connection_str) {
  switch (type) {
    case MapDriverType::SQLITE:
      return std::make_unique<MapInterfaceSqlite3>(connection_str);
    case MapDriverType::POSTGRESQL:
#if HAS_PQXX
      return std::make_unique<MapInterfacePostgresql>(connection_str);
#else
      throw DatabaseError("Postgresql support not compiled in.");
#endif
  }

  std::string err("Invalid database driver type: ");
  err += std::to_string(type);
  throw DatabaseError(err);
}
