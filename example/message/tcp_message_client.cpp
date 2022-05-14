#include "salt/packet_assemble/header_body_assemble.h"
#include "salt/util/byte_order.h"

#include <algorithm>
#include <iterator>

#pragma pack(1)
class message_header {
public:
  uint32_t magic_;
  uint16_t len_;
};
#pragma pack()

std::string encode_with_string(message_header &header, const std::string &s) {
  std::string result;
  result.reserve(sizeof(message_header));
  std::copy(reinterpret_cast<char *>(&header.magic_),
            reinterpret_cast<char *>(&header.magic_) + sizeof(header.magic_),
            std::back_inserter(result));

  header.len_ = s.size() + sizeof(header.len_);
  header.len_ = salt::byte_order::to_network(header.len_);

  std::copy(reinterpret_cast<char *>(&header.len_),
            reinterpret_cast<char *>(&header.len_) + sizeof(decltype(header.len_)),
            std::back_inserter(result));

  result += s;
  return result;
}

int main() {
  auto packet_assemble =
      salt::header_body_assemble<message_header, &message_header::len_>();
  packet_assemble.body_length_calc_mode_ =
      salt::body_length_calc_mode::with_length_field;

  message_header h;
  h.magic_ = 12345;
  auto s = encode_with_string(h, "hello");
  s += encode_with_string(h, ", world.");
  (&packet_assemble)->data_received(nullptr, s.substr(0, 3));
  (&packet_assemble)->data_received(nullptr, s.substr(3, 6));
  (&packet_assemble)->data_received(nullptr, s.substr(9, 5));
  (&packet_assemble)->data_received(nullptr, s.substr(14));
  return 0;
}