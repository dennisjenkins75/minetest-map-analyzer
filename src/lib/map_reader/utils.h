#ifndef _MT_MAP_SEARCH_UTILS_H_
#define _MT_MAP_SEARCH_UTILS_H_

#include <string>

static constexpr bool DEBUG = false;

static constexpr char CLEAR[] = "\x1b[0m";
static constexpr char RED[] = "\x1b[31m";
static constexpr char GREEN[] = "\x1b[32m";
static constexpr char YELLOW[] = "\x1b[33m";
static constexpr char CYAN[] = "\x1b[35m";

// No embedded ANSI color codes, all hex on one line.
std::string to_hex(const void *data, size_t bytes);

// Emits multi-line block of text, 16 bytes per line, with ASCII chars.
// Embeds ANSI color codes.
std::string to_hex_block(const void *data, size_t bytes);

#endif // _MT_MAP_SEARCH_UTILS_H_
