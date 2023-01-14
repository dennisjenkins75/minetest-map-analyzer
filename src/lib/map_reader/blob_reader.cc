#include <zlib.h>
#include <zstd.h>

#include <iostream>

#include "blob_reader.h"
#include "utils.h"

static constexpr size_t BUFFER_SIZE = 1024 * 32;

bool BlobReader::read_line(std::string *dest, const std::string_view desc) {
  const char *start = reinterpret_cast<const char *>(_ptr);

  while (size_check(1, desc)) {
    if (isprint(*_ptr)) {
      _ptr++;
      continue;
    } else if (*_ptr == '\n') {
      dest->assign(start, reinterpret_cast<const char *>(_ptr) - start);
      _ptr++;
      return true;
    }
    // garbage data?
    break;
  }
  return false;
}

bool BlobReader::decompress_zlib(std::vector<uint8_t> *dest,
                                 const std::string_view desc) {
  z_stream z{};
  int status = 0;
  int ret = 0;
  int bytes_read = 0;
  int bytes_written = 0;
  bool result = false;

  dest->clear();
  dest->reserve(BUFFER_SIZE);

  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;
  z.avail_in = 0;

  ret = inflateInit(&z);
  if (ret != Z_OK) {
    std::cout << "decompress_zlib(), inflateInit() failed.\n";
    return false;
  }

  while (true) {
    // Do we have anything to read?
    if (!z.avail_in) {
      z.next_in = const_cast<uint8_t *>(ptr());
      z.avail_in = &*_blob.end() - ptr();

      if (DEBUG) {
        std::cout << "z.next_in = " << static_cast<const void *>(z.next_in)
                  << "\n";
        std::cout << "z.avail_in = " << z.avail_in << "\n";
      }
    }

    if (z.avail_in <= 0) {
      break;
    }
    const size_t src_orig_size = z.avail_in;

    // Make room to write to the output vector.
    // Note that we are writing directly to the output vector.
    dest->resize(dest->size() + BUFFER_SIZE);
    z.avail_out = BUFFER_SIZE;
    z.next_out = dest->data() + z.total_out;

    // Decompress.
    status = inflate(&z, Z_NO_FLUSH);
    bytes_read = src_orig_size - z.avail_in;
    bytes_written = BUFFER_SIZE - z.avail_out;

    if (DEBUG) {
      std::cout << "inflate: status: " << status << ", read: " << bytes_read
                << ", written: " << bytes_written << "\n";
    }

    if ((status != Z_STREAM_END) && (status != Z_OK)) {
      std::cout << "zlib error: " << status << ", " << (z.msg ? z.msg : "")
                << "\n";
      return false;
    }

    // Update input stream pointer.
    if (!skip(bytes_read, "zlib.inflate")) {
      break;
    }

    if (status == Z_STREAM_END) {
      // Update output buffer size/end.  "dest->size()" is likely bigger than
      // the amount of data that we actually have.
      dest->resize(z.total_out);

      result = true;
      break;
    }
  }

  inflateEnd(&z);
  return result;
}
