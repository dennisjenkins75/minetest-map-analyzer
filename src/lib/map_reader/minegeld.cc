#include <cstdlib>
#include <cstring>

#include "src/lib/map_reader/minegeld.h"

// "currency:minegeld_10"
// "currency:minegeld_25 46"

static constexpr char kMinegeldPrefix[] = "currency:minegeld_";

uint64_t ParseCurrencyMinegeld(const std::string &input) {
#if defined(__cpp_lib_starts_ends_with)
  // C++20 feature.
  if (!input.starts_with(kMinegeldPrefix)) {
#else
  if (strncmp(input.c_str(), kMinegeldPrefix, sizeof(kMinegeldPrefix) - 1)) {
#endif
    return 0;
  }

  const char *startp = input.c_str() + sizeof(kMinegeldPrefix) - 1;
  char *endp = nullptr;
  const uint64_t denomination = strtoul(startp, &endp, 10);
  if (!denomination) {
    return 0;
  }

  if (!*endp) {
    return denomination;
  }
  while (*endp == ' ') {
    ++endp;
  }
  const uint64_t qty = strtoul(endp, nullptr, 10);
  if (!qty) {
    return 0; // error?
  }

  return denomination * qty;
}
