// Utility class that holds a const reference to a vector of data.
// Provides handy accessors for reading this data, and converting it from
// big-endian format to machine-native format.

#pragma once

#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "src/lib/exceptions/exceptions.h"

class SerializationError : public Error {
public:
  SerializationError() = delete;
  SerializationError(std::string_view msg) : Error(msg) {}
};

class BlobReader {
public:
  BlobReader() = delete;
  explicit BlobReader(const std::vector<uint8_t> &blob)
      : _blob(blob), _ptr(blob.data()) {}

  const std::vector<uint8_t> &data() const { return _blob; }
  const uint8_t *ptr() const { return _ptr; }

  size_t size() const { return _blob.size(); }
  size_t remaining() const { return &*_blob.end() - _ptr; }
  size_t offset() const { return _ptr - &*_blob.begin(); }
  bool eof() const { return !remaining(); }

  // Throws error is the amount of requested bytes is not available.

  void size_check(size_t bytes, const std::string_view desc) const {
    if ((_ptr + bytes) <= &(*_blob.end())) {
      return;
    }

    std::stringstream ss;
    ss << "BlobReader size_check(" << bytes << " '" << desc
       << "') failed.  blob.size: " << size() << ", blob.offset: " << offset()
       << ", blob.remaining: " << remaining();

    throw SerializationError(ss.str());
  }

  // Skips 'bytes' forwards, if doing so won't go beyond end of the blob.
  void skip(size_t bytes, const std::string_view desc) {
    size_check(bytes, desc);
    _ptr += bytes;
  }

  uint8_t read_u8(const std::string_view desc) {
    size_check(sizeof(uint8_t), desc);
    uint8_t ret = *reinterpret_cast<const uint8_t *>(_ptr);
    _ptr += sizeof(uint8_t);
    return ret;
  }

  uint16_t read_u16(const std::string_view desc) {
    size_check(sizeof(uint16_t), desc);
    uint16_t ret = ntohs(*reinterpret_cast<const uint16_t *>(_ptr));
    _ptr += sizeof(uint16_t);
    return ret;
  }

  int32_t read_s32(const std::string_view desc) {
    size_check(sizeof(int32_t), desc);
    int32_t ret = ntohl(*reinterpret_cast<const int32_t *>(_ptr));
    _ptr += sizeof(int32_t);
    return ret;
  }

  uint32_t read_u32(const std::string_view desc) {
    size_check(sizeof(uint32_t), desc);
    uint32_t ret = ntohl(*reinterpret_cast<const uint32_t *>(_ptr));
    _ptr += sizeof(uint32_t);
    return ret;
  }

  // TODO: Can re return a string_view instead?
  std::string read_str(uint32_t len, const std::string_view desc) {
    size_check(len, desc);
    std::string ret(reinterpret_cast<const char *>(_ptr), len);
    _ptr += len;
    return ret;
  }

  // Strips off trailing "\n".
  // Returns false when there is no data left in the blob.
  // TODO: Return raw string and use exception to indicate blob overrun.
  bool read_line(std::string *dest, const std::string_view desc);

  // Assume that `_ptr` points to a zlib compressed block.  Return decompressed
  // data and update this->_ptr to point to the first byte after the zlib
  // compressed stream ends.
  // Will throw an exception if decompression fails.
  std::vector<uint8_t> decompress_zlib(const std::string_view desc);

private:
  const std::vector<uint8_t> &_blob;
  const uint8_t *_ptr;
};
