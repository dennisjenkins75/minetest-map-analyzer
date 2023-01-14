#include <iostream>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include "src/lib/database/db-sqlite3.h"

SqliteStmt::SqliteStmt(SqliteDb &db, const std::string &sql)
    : db_(db), stmt_(), sql_(sql) {
  sqlite3_stmt *stmt = nullptr;

  int rc = sqlite3_prepare_v2(db_ptr(), sql.c_str(), -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    throw Sqlite3Error(rc, sqlite3_errmsg(db_ptr()), "sqlite3_prepare_v2", sql);
  }
  stmt_ = StmtPointer(stmt);
}

bool SqliteStmt::Step() {
  int rc = sqlite3_step(stmt_.get());

  switch (rc) {
    case SQLITE_OK:
    case SQLITE_ROW:
      return true;
    case SQLITE_DONE:
      return false;
  }

  throw Sqlite3Error(rc, sqlite3_errmsg(db_ptr()), "sqlite3_step",
                     sqlite3_sql(stmt_.get()));
}

void SqliteStmt::Reset() {
  // https://www.sqlite.org/c3ref/reset.html
  // meh, don't care about return value.
  sqlite3_reset(stmt_.get());
}

void SqliteStmt::BindInt(int index, int64_t value) {
  int r = sqlite3_bind_int64(stmt_.get(), index, value);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(db_ptr()), "sqlite3_bind_int64",
                       sqlite3_sql(stmt_.get()));
  }
}

void SqliteStmt::BindBlob(int index, const void *p, size_t size) {
  int r = sqlite3_bind_blob(stmt_.get(), index, p, size, nullptr);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(db_ptr()), "sqlite3_bind_blob",
                       sqlite3_sql(stmt_.get()));
  }
}

void SqliteStmt::BindText(int index, const std::string_view &text) {
  int r = sqlite3_bind_text(stmt_.get(), index, text.data(), text.size(),
                            SQLITE_STATIC);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(db_ptr()), "sqlite3_bind_text",
                       sqlite3_sql(stmt_.get()));
  }
}

std::vector<uint8_t> SqliteStmt::ColumnBlob(int index) {
  const uint8_t *data =
      static_cast<const uint8_t *>(sqlite3_column_blob(stmt_.get(), index));
  const size_t data_len = sqlite3_column_bytes(stmt_.get(), index);

  return std::vector<uint8_t>(data, data + data_len);
}

SqliteDb::SqliteDb(std::string_view connection_str)
    : connection_str_(connection_str), database_(), stmt_begin_(),
      stmt_commit_(), stmt_rollback_() {

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

  // TODO: Call sqlite3_busy_handler()

  stmt_begin_ = std::make_unique<SqliteStmt>(*this, "begin");
  stmt_commit_ = std::make_unique<SqliteStmt>(*this, "end");
  stmt_rollback_ = std::make_unique<SqliteStmt>(*this, "rollback");
}

SqliteDb::~SqliteDb() {
  // Can't throw errors here, they call std::terminate().
  sqlite3_close_v2(database_.get());
}

std::string SqliteDb::GetVersionInfo() const {
  return std::string("sqlite3-") + std::string(sqlite3_libversion());
}

void SqliteDb::Begin() {
  stmt_begin_->Step();
  stmt_begin_->Reset();
}

void SqliteDb::Commit() {
  stmt_commit_->Step();
  stmt_commit_->Reset();
}

void SqliteDb::Rollback() {
  stmt_rollback_->Step();
  stmt_rollback_->Reset();
}

void SqliteDb::Exec(const std::string &sql) {
  int r = sqlite3_exec(database_.get(), sql.c_str(), nullptr, nullptr, nullptr);
  if (r != SQLITE_OK) {
    throw Sqlite3Error(r, sqlite3_errmsg(database_.get()), "sqlite3_exec", sql);
  }
}
