#pragma once

#include <sqlite3.h>

#include <chrono>
#include <memory>

#include "src/lib/exceptions/exceptions.h"

class Sqlite3Error : public DatabaseError {
public:
  Sqlite3Error() = delete;

  explicit Sqlite3Error(int result, std::string_view msg,
                        std::string_view sqlite3_api, std::string_view sql)
      : DatabaseError(msg), result_(result), sqlite3_api_(sqlite3_api),
        sql_(sql) {}

protected:
  int result_;
  std::string sqlite3_api_;
  std::string sql_;
};

class DatabaseSqlite3 {
public:
  DatabaseSqlite3() = delete;

  explicit DatabaseSqlite3(std::string_view connection_str);
  virtual ~DatabaseSqlite3();

  // https://stackoverflow.com/a/45215544
  struct sqlite3_deleter {
    void operator()(sqlite3 *db) {
      if (db) {
        // NOTE: ignores errors (SQLITE_BUSY).
        sqlite3_close_v2(db);
      }
    }
  };
  using DatabasePointer = std::unique_ptr<sqlite3, sqlite3_deleter>;

  struct stmt_deleter {
    void operator()(sqlite3_stmt *stmt) {
      if (stmt) {
        sqlite3_finalize(stmt);
      }
    }
  };
  using StmtPointer = std::unique_ptr<sqlite3_stmt, stmt_deleter>;

  // Wrappers around sqlite3 C-level API.
  // These all throw `Sqlite3Error` on any error.
  std::string GetVersionInfo() const;
  void Open();
  void Close();
  void Exec(const std::string &sql);
  StmtPointer Prepare(const std::string &sql);

  void Begin();
  void Commit();
  void Rollback();

  // Returns 'true' if step fetched a row, 'false' on SQLITE_DONE, throws
  // exception on error.
  bool Step(sqlite3_stmt *stmt);
  void Reset(sqlite3_stmt *stmt);
  void BindInt(sqlite3_stmt *stmt, int index, int64_t value);
  void BindBlob(sqlite3_stmt *stmt, int index, const void *p, size_t size);
  void BindText(sqlite3_stmt *stmt, int index, const std::string_view &text);

  // We compute the timestamp in C++ b/c its faster than asking the sqlite3
  // parser to do it.
  int64_t Now() const {
    return std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
  }

  int64_t ColumnInt64(sqlite3_stmt *stmt, int index) {
    return sqlite3_column_int64(stmt, index);
  }

  std::string ColumnText(sqlite3_stmt *stmt, int index) {
    return std::string(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, index)));
  }

  std::vector<uint8_t> ColumnBlob(sqlite3_stmt *stmt, int index);

protected:
  std::string connection_str_;

  // The order of these unique_ptr's is SIGNIFICANT.  We need the statements
  // destroyed in reverse order of creation.
  DatabasePointer database_;
  StmtPointer stmt_begin_;
  StmtPointer stmt_commit_;
  StmtPointer stmt_rollback_;
};
