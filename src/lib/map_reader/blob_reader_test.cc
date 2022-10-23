#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/lib/map_reader/blob_reader.h"

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Ne;
using ::testing::Pointwise;
using ::testing::SizeIs;
using ::testing::StrEq;

TEST(BlobReader, General) {
  const std::vector<uint8_t> input = {
      0x3a, 0x2b, // u16 (0x3a2b)
      0xff,       // u8 (0xff)
      0x7f,       // u8 (0x7f)

      0xde, 0xad, 0xbe, 0xef, // u32 (0xdeadbeef)
      0x05,                   // u8 (string len = 5)
      'h',  'e',  'l',  'l',  // "hello"
      'o',
  };

  BlobReader r(input);
  EXPECT_THAT(r.eof(), IsFalse());
  EXPECT_THAT(r, SizeIs(14));
  EXPECT_THAT(r.remaining(), Eq(14));
  EXPECT_THAT(r.size_check(13, "foo"), IsTrue());
  EXPECT_THAT(r.size_check(14, "foo"), IsTrue());
  EXPECT_THAT(r.size_check(15, "foo"), IsFalse());

  // Begin reading, which will mutate state.
  uint16_t a = 0;
  EXPECT_THAT(r.read_u16(&a, "foo"), IsTrue());
  EXPECT_THAT(a, Eq(0x3a2b));
  EXPECT_THAT(r.remaining(), Eq(12));

  uint8_t b = 0;
  EXPECT_THAT(r.read_u8(&b, "foo"), IsTrue());
  EXPECT_THAT(b, Eq(255));
  EXPECT_THAT(r.remaining(), Eq(11));

  uint8_t c = 0;
  EXPECT_THAT(r.read_u8(&c, "foo"), IsTrue());
  EXPECT_THAT(c, Eq(127));
  EXPECT_THAT(r.remaining(), Eq(10));

  uint32_t d = 0;
  EXPECT_THAT(r.read_u32(&d, "bigfoo"), IsTrue());
  EXPECT_THAT(d, Eq(0xdeadbeef));
  EXPECT_THAT(r.remaining(), Eq(6));

  uint8_t len = 0;
  EXPECT_THAT(r.read_u8(&len, "strlen"), IsTrue());
  EXPECT_THAT(len, Eq(5));
  EXPECT_THAT(r.remaining(), Eq(5));

  std::string s;
  EXPECT_THAT(r.read_str(&s, len, "str"), IsTrue());
  EXPECT_THAT(s, Eq("hello"));
  EXPECT_THAT(r.remaining(), Eq(0));

  // We should have reached EOF.
  EXPECT_THAT(r.eof(), IsTrue());

  uint8_t z = 0;
  EXPECT_THAT(r.read_u8(&z, "eof"), IsFalse());
}

// TODO: Need unit test for `BlobReader::decompress_zlib()`
// TODO: Need unit test for `BlobReader::read_line()`
