#include <sstream>

#include "utils.h"

static inline std::string h8(uint8_t value) {
  char b[8];
  snprintf(b, sizeof(b), "%02x", value);
  return std::string(b);
}

static inline std::string h16(uint16_t value) {
  char b[8];
  snprintf(b, sizeof(b), "%04x", value);
  return std::string(b);
}

std::string to_hex(const void *data, size_t byte_count) {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(data);

  std::string r;
  r.reserve(byte_count * 3 + 1);

  for (size_t i = 0; i < byte_count; i++) {
    char tmp[8];
    snprintf(tmp, sizeof(tmp), "%02x ", p[i]);
    r += tmp;
  }

  if (byte_count > 0) {
    r.resize(r.size() - 1);
  }

  return r;
}

std::string to_hex_block(const void *ptr, size_t byte_count) {
  std::stringstream ss;

  const uint8_t *p = reinterpret_cast<const uint8_t *>(ptr);

  // Dump in rows of 16 bytes each, until we hit our `bytes` limit.
  constexpr size_t STRIDE = 16;
  for (size_t offset = 0; offset < byte_count; offset += STRIDE) {
    const size_t values = std::min(STRIDE, byte_count - offset);
    const size_t spacer = STRIDE - values;

    // print offset.
    ss << YELLOW << h16(offset) << ": ";

    // print hex values.
    for (size_t i = 0; i < values; i++) {
      ss << (isprint(p[i]) ? GREEN : CLEAR);
      ss << h8(p[i]) << " ";
    }
    // print spacers.
    for (size_t i = 0; i < spacer; i++) {
      ss << "   ";
    }

    ss << "- ";

    for (size_t i = 0; i < values; i++) {
      if (isprint(p[i])) {
        ss << GREEN << p[i];
      } else {
        ss << CLEAR << " ";
      }
    }
    ss << CLEAR << "\n";

    p += STRIDE;
  }

  return ss.str();
}
