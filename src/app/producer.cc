#include "src/app/producer.h"
#include "src/app/database.h"

void Producer(MapBlockQueue *queue, sqlite3 *db, const MapBlockPos &min,
              const MapBlockPos &max) {
  const char sql[] = "select pos, mtime from blocks "
                     "where pos between :min_pos and :max_pos "
                     "order by pos";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  handle_sqlite3_error(db, rc);

  rc = sqlite3_bind_int64(stmt, 1, min.MapBlockId());
  handle_sqlite3_error(db, rc);

  rc = sqlite3_bind_int64(stmt, 2, max.MapBlockId());
  handle_sqlite3_error(db, rc);

  while (true) {
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
      break;
    }

    MapBlockKey key{
        .pos = sqlite3_column_int64(stmt, 0),
        .mtime = sqlite3_column_int64(stmt, 1),
    };

    if (MapBlockPos(key.pos).inside(min, max)) {
      queue->Enqueue(std::move(key));
    }
  }

  rc = sqlite3_finalize(stmt);
  handle_sqlite3_error(db, rc);
}
