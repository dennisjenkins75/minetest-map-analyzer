#pragma once

#include <string_view>

extern "C" {
extern const char _binary____src_app_schema_schema_data_sql_start;
extern const char _binary____src_app_schema_schema_data_sql_end;
}

static inline const std::string_view GetSqlSchema() {
  return std::string_view(&_binary____src_app_schema_schema_data_sql_start,
                          &_binary____src_app_schema_schema_data_sql_end -
                              &_binary____src_app_schema_schema_data_sql_start);
}

// Attempt to open/create filename as sqlite database.
// Check if an expected table exists, and if not, run the entire schema
// script on it.  Dies on exception if this fails.
void VerifySchema(const std::string &filename);
