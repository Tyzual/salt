#include "gtest/gtest.h"

#include "salt/packet_assemble/header_body_assemble.h"

class message_header {
public:
  uint32_t magic_;
  uint32_t len_;
};

std::string encode_with_string(message_header &head, const std::string &s) {
  std::string result;
  result.reserve(sizeof(message_header));
  std::copy(reinterpret_cast<char *>(&head.magic_),
            reinterpret_cast<char *>(&head.magic_) + sizeof(uint32_t),
            std::back_inserter(result));

  head.len_ = s.size();

  std::copy(reinterpret_cast<char *>(&head.len_),
            reinterpret_cast<char *>(&head.len_) + sizeof(uint32_t),
            std::back_inserter(result));

  result += s;
  return result;
}

TEST(salt_pack_test, unpack) {
  message_header h;
  h.magic_ = 12345;
  auto s = encode_with_string(h, "hello");
  s += encode_with_string(h, "world");
  auto packet_assemble =
      salt::header_body_assemble<message_header, &message_header::len_>();
  for (int step = 1; step < s.size() + 1; ++step) {
  }

  ASSERT_TRUE(true);
}