// Utility class that holds a const reference to a vector of data.
// Provides handy accessors for reading this data, and converting it from
// big-endian format to machine-native format.

#pragma once

#include <arpa/inet.h>
#include <string>
#include <string_view>
#include <vector>

class BlobReader {
public:
  BlobReader() = delete;
  explicit BlobReader(const std::vector<uint8_t> &blob)
      : _blob(blob), _ptr(blob.data()) {}

  const std::vector<uint8_t> &data() const { return _blob; }
  const uint8_t *ptr() const { return _ptr; }

  size_t size() const { return _blob.size(); }
  size_t remaining() const { return &*_blob.end() - _ptr; }
  bool eof() const { return !remaining(); }

  // Return 'true' if there are atleast `bytes` count of bytes available to
  // read.  Typically, returning 'false' indicates an error.
  bool size_check(size_t bytes, const std::string_view desc) const {
    return (_ptr + bytes) <= &(*_blob.end());
  }

  // Skips 'bytes' forwards, if doing so won't go beyond end of the blob.
  bool skip(size_t bytes, const std::string_view desc) {
    return size_check(bytes, desc) && (_ptr += bytes);
  }

  bool read_u8(uint8_t *dest, const std::string_view desc) {
    return size_check(sizeof(uint8_t), desc) &&
           (*dest = *reinterpret_cast<const uint8_t *>(_ptr),
            _ptr += sizeof(uint8_t));
  }

  bool read_u16(uint16_t *dest, const std::string_view desc) {
    return size_check(sizeof(uint16_t), desc) &&
           (*dest = ntohs(*reinterpret_cast<const uint16_t *>(_ptr)),
            _ptr += sizeof(uint16_t));
  }

  bool read_s32(int32_t *dest, const std::string_view desc) {
    return size_check(sizeof(int32_t), desc) &&
           (*dest = ntohl(*reinterpret_cast<const int32_t *>(_ptr)),
            _ptr += sizeof(int32_t));
  }

  bool read_u32(uint32_t *dest, const std::string_view desc) {
    return size_check(sizeof(uint32_t), desc) &&
           (*dest = ntohl(*reinterpret_cast<const uint32_t *>(_ptr)),
            _ptr += sizeof(uint32_t));
  }

  bool read_str(std::string *dest, uint32_t len, const std::string_view desc) {
    return size_check(len, desc) &&
           (dest->assign(reinterpret_cast<const char *>(_ptr), len),
            _ptr += len);
  }

  // Strips off trailing "\n".
  bool read_line(std::string *dest, const std::string_view desc);

  // Assume that `_ptr` points to a zlib compressed block.  Decompress it
  // into `dest` (overwriting anything already there), and update this->_ptr to
  // point to the first byte after the zlib compressed data.
  bool decompress_zlib(std::vector<uint8_t> *dest, const std::string_view desc);

private:
  const std::vector<uint8_t> &_blob;
  const uint8_t *_ptr;
};
