#pragma once

#include <exception>
#include <string>
#include <string_view>

// Avoid throwing `Error` directly, and prefer to throw a derived exception
// class instead.
class Error : public std::exception {
public:
  Error() = delete;

  explicit Error(std::string_view msg) : msg_(msg) {}

  virtual ~Error() noexcept {}

  virtual const char *what() const noexcept { return msg_.c_str(); }

protected:
  std::string msg_;

  // TODO: Capture stack trace (C++ and Lua) during constructor, store it here.
};

// Each database backend should define their own error class.
class DatabaseError : public Error {
public:
  DatabaseError() = delete;

  explicit DatabaseError(std::string_view msg) : Error(msg) {}
};

class UnimplementedError : public Error {
public:
  UnimplementedError() = delete;
  explicit UnimplementedError(std::string_view msg) : Error(msg) {}
};
