// Utility class that holds a const reference to a vector of data.
// Provides handy accessors for reading this data, and converting it from
// big-endian format to machine-native format.

#ifndef _MT_MAP_SEARCH_BLOB_READER_H
#define _MT_MAP_SEARCH_BLOB_READER_H

#include <arpa/inet.h>
#include <string_view>
#include <string>
#include <vector>

class BlobReader {
public:
  BlobReader() = delete;
  explicit BlobReader(const std::vector<uint8_t> &blob) :
    _blob(blob), _ptr(blob.data())
    {}

  const std::vector<uint8_t>& data() const { return _blob; }
  const uint8_t* ptr() const { return _ptr; }

  size_t size() const { return _blob.size(); }
  size_t remaining() const { return &*_blob.end() - _ptr; }

  // Return 'true' if there are atleast `bytes` count of bytes available to
  // read.
  bool size_check(size_t bytes, std::string_view desc) const;
  bool skip(size_t bytes, std::string_view desc);

  bool read_s8(int8_t *dest, std::string_view desc);
  bool read_ch(char *dest, std::string_view desc) {
    return read_s8(reinterpret_cast<int8_t*>(dest), desc);
  }
  bool read_u8(uint8_t *dest, std::string_view desc);
  bool read_s16(int16_t *dest, std::string_view desc);
  bool read_u16(uint16_t *dest, std::string_view desc);
  bool read_s32(int32_t *dest, std::string_view desc);
  bool read_u32(uint32_t *dest, std::string_view desc);
  bool read_str(std::string *dest, uint32_t len, std::string_view desc);

  // Strips off trailing "\n".
  bool read_line(std::string *dest, std::string_view desc);

  // Assume that `_ptr` points to a zlib compressed block.  Decompress it
  // into `dest` (overwriting anything already there), and update this->_ptr to
  // point to the first byte after the zlib compressed data.
  bool decompress_zlib(std::vector<uint8_t> *dest, std::string_view desc);

private:
  const std::vector<uint8_t> &_blob;
  const uint8_t * _ptr;
};

#endif  // _MT_MAP_SEARCH_BLOB_READER_H
