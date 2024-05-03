#include <string.h>
#include "../testing.h"
#include "test_codec.h"
#include <mcpr/codec.h>

void test_mcpr_encode_varint(void **state)
{
  uint8_t buf[5] = { 0 };

  int32_t input_array[] = {
    0,
    1,
    2,
    127,
    128,
    255,
    25565,
    2097151,
    2147483647,
    -1,
    -2147483648
  };

  uint8_t expected_array[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x01, 0x00, 0x00, 0x00, 0x00 },
    { 0x02, 0x00, 0x00, 0x00, 0x00 },
    { 0x7f, 0x00, 0x00, 0x00, 0x00 },
    { 0x80, 0x01, 0x00, 0x00, 0x00 },
    { 0xff, 0x01, 0x00, 0x00, 0x00 },
    { 0xdd, 0xc7, 0x01, 0x00, 0x00 },
    { 0xff, 0xff, 0x7f, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0xff, 0x07 },
    { 0xff, 0xff, 0xff, 0xff, 0x0f },
    { 0x80, 0x80, 0x80, 0x80, 0x08 }
  };

  size_t expected_lengths[] = {
    1,
    1,
    1,
    1,
    2,
    2,
    3,
    3,
    5,
    5,
    5
  };

  for (int i = 0; i < 11; i++)
  {
    int32_t input = input_array[i];
    uint8_t *expected = expected_array[i];
    size_t expected_length = expected_lengths[i];
    ssize_t result = mcpr_encode_varint(buf, input);
    assert_in_range(result, 0, expected_length);
    assert_true(memcmp(buf, expected, 5) == 0);
    memset(buf, 0x00, 5);
  }
}
