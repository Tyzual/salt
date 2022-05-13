#include "salt/packet_assemble/header_body_assemble.h"

#include <algorithm>
#include <iterator>

class MessageHead {
public:
  uint32_t magic_;
  uint32_t len_;
};

std::string encode_with_string(MessageHead &head, const std::string &s) {
  std::string result;
  result.reserve(sizeof(MessageHead));
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

int main() {
  auto packet_assemble =
      salt::header_body_assemble<MessageHead, uint32_t, &MessageHead::len_>();

  MessageHead h;
  h.magic_ = 12345;
  auto s = encode_with_string(h, "hello");
  s += encode_with_string(h, ", world.");
  (&packet_assemble)->data_received(nullptr, s);
  return 0;
}