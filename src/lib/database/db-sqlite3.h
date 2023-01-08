#pragma once

#include <sqlite3.h>

#include <chrono>
#include <memory>
#include <vector>

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

class SqliteStmt;
class SqliteDb {
  friend class SqliteStmt;

public:
  SqliteDb() = delete;

  explicit SqliteDb(std::string_view connection_str);
  virtual ~SqliteDb();

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

  // Wrappers around sqlite3 C-level API.
  // These all throw `Sqlite3Error` on any error.
  std::string GetVersionInfo() const;
  void Exec(const std::string &sql);

  void Begin();
  void Commit();
  void Rollback();

protected:
  std::string connection_str_;

  // The order of these unique_ptr's is SIGNIFICANT.  We need the statements
  // destroyed in reverse order of creation.
  DatabasePointer database_;

  std::unique_ptr<SqliteStmt> stmt_begin_;
  std::unique_ptr<SqliteStmt> stmt_commit_;
  std::unique_ptr<SqliteStmt> stmt_rollback_;
};

class SqliteStmt {
public:
  SqliteStmt(SqliteDb &db, const std::string &sql);
  ~SqliteStmt() {}

  // Returns 'true' if step fetched a row, 'false' on SQLITE_DONE, throws
  // exception on error.
  bool Step();

  void Reset();

  void BindInt(int index, int64_t value);
  void BindBlob(int index, const void *p, size_t size);
  void BindText(int index, const std::string_view &text);

  int64_t ColumnInt64(int index) {
    return sqlite3_column_int64(stmt_.get(), index);
  }

  std::string ColumnText(int index) {
    return std::string(reinterpret_cast<const char *>(
        sqlite3_column_text(stmt_.get(), index)));
  }

  std::vector<uint8_t> ColumnBlob(int index);

private:
  struct stmt_deleter {
    void operator()(sqlite3_stmt *stmt) {
      if (stmt) {
        sqlite3_finalize(stmt);
      }
    }
  };
  using StmtPointer = std::unique_ptr<sqlite3_stmt, stmt_deleter>;

  SqliteDb &db_;
  StmtPointer stmt_;
  std::string sql_;

  sqlite3 *db_ptr() { return db_.database_.get(); }
};
