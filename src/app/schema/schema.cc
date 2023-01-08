#include <spdlog/spdlog.h>

#include "src/app/schema/schema.h"
#include "src/lib/database/db-sqlite3.h"

static constexpr char kSqlCheckSchema[] = R"sql(
select count(1)
from sqlite_schema
where type='table' and name='actor';
)sql";

void VerifySchema(const std::string &filename) {
  SqliteDb db(filename);

  SqliteStmt stmt(db, kSqlCheckSchema);
  stmt.Step();

  const int count = stmt.ColumnInt64(0);
  if (count > 0) {
    return;
  }
  stmt.Reset();

  spdlog::info("Created schema in {0}", filename);
  db.Begin();
  db.Exec(std::string(GetSqlSchema()));
  db.Commit();
}
