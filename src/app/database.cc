#include <iostream>
#include <unistd.h>

#include "src/app/database.h"

void handle_sqlite3_error(sqlite3 *db, int rc) {
  if (rc != SQLITE_OK) {
    std::cout << "Fatal Sqlite error: " << rc << ", " << sqlite3_errstr(rc)
              << "\n";
    exit(EXIT_FAILURE);
  }
}
