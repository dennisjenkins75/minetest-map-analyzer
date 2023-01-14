#include "src/lib/map_reader/utils.h"

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
