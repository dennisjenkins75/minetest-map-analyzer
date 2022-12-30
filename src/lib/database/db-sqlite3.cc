#include <absl/strings/substitute.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include "src/lib/database/db-sqlite3.h"

DatabaseSqlite3::DatabaseSqlite3(std::string_view connection_str)
    : connection_str_(connection_str), database_(), stmt_begin_(),
      stmt_commit_(), stmt_rollback_() {
  Open();
  // TODO: Call sqlite3_busy_handler()

  stmt_begin_ = Prepare("begin");
  stmt_commit_ = Prepare("end");
  stmt_rollback_ = Prepare("rollback");
}

DatabaseSqlite3::~DatabaseSqlite3() { Close(); }

std::string DatabaseSqlite3::GetVersionInfo() const {
  return std::string("sqlite3-") + std::string(sqlite3_libversion());
}

void DatabaseSqlite3::Begin() {
  Step(stmt_begin_.get());
  Reset(stmt_begin_.get());
}

void DatabaseSqlite3::Commit() {
  Step(stmt_commit_.get());
  Reset(stmt_commit_.get());
}

void DatabaseSqlite3::Rollback() {
  Step(stmt_rollback_.get());
  Reset(stmt_rollback_.get());
}

void DatabaseSqlite3::Open() {
  // For now, treat connection_str as a raw filename.
  // Possible future change: treat it as an sqlite3 "URI", so that we can
  // reuse this code to handle a "read-only" open (via URI params).

  sqlite3 *db = nullptr;
  int r = sqlite3_open_v2(connection_str_.c_str(), &db,
                          SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r,
                       std::string("Failed to create/open sqlite3 database ") +
                           connection_str_,
                       "sqlite3_open_v2", "");
  }

  sqlite3_extended_result_codes(db, 1);
  database_.reset(db);

  spdlog::debug("Database opened: {0} {1}", connection_str_, GetVersionInfo());
}

void DatabaseSqlite3::Close() {
  int r = sqlite3_close_v2(database_.get());
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_close_v2",
                       "");
  }
  database_.reset(nullptr);
}

void DatabaseSqlite3::Exec(const std::string &sql) {
  int r = sqlite3_exec(database_.get(), sql.c_str(), nullptr, nullptr, nullptr);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_exec", sql);
  }
}

DatabaseSqlite3::StmtPointer DatabaseSqlite3::Prepare(const std::string &sql) {
  sqlite3_stmt *stmt = nullptr;
  int r = sqlite3_prepare_v2(database_.get(), sql.c_str(), -1, &stmt, nullptr);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_prepare_v2",
                       sql);
  }
  return StmtPointer(stmt);
}

bool DatabaseSqlite3::Step(sqlite3_stmt *stmt) {
  int r = sqlite3_step(stmt);
  switch (r) {
    case SQLITE_OK:
    case SQLITE_ROW:
      return true;
    case SQLITE_DONE:
      return false;
  }

  throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_step",
                     sqlite3_sql(stmt));
}

void DatabaseSqlite3::Reset(sqlite3_stmt *stmt) {
  // https://www.sqlite.org/c3ref/reset.html
  // meh, don't care about return value.
  sqlite3_reset(stmt);
}

void DatabaseSqlite3::BindInt(sqlite3_stmt *stmt, int index, int64_t value) {
  int r = sqlite3_bind_int64(stmt, index, value);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_bind_int64",
                       sqlite3_sql(stmt));
  }
}

void DatabaseSqlite3::BindBlob(sqlite3_stmt *stmt, int index, const void *p,
                               size_t size) {
  int r = sqlite3_bind_blob(stmt, index, p, size, nullptr);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_bind_blob",
                       sqlite3_sql(stmt));
  }
}

void DatabaseSqlite3::BindText(sqlite3_stmt *stmt, int index,
                               const std::string_view &text) {
  int r =
      sqlite3_bind_text(stmt, index, text.data(), text.size(), SQLITE_STATIC);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_bind_text",
                       sqlite3_sql(stmt));
  }
}

std::vector<uint8_t> DatabaseSqlite3::ColumnBlob(sqlite3_stmt *stmt,
                                                 int index) {
  const uint8_t *data =
      static_cast<const uint8_t *>(sqlite3_column_blob(stmt, index));
  const size_t data_len = sqlite3_column_bytes(stmt, index);

  return std::vector<uint8_t>(data, data + data_len);
}
