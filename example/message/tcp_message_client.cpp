#include "salt/core/tcp_client.h"
#include "salt/packet_assemble/header_body_assemble.h"
#include "salt/util/byte_order.h"

#include "util/string_util.h"

#include <algorithm>
#include <iostream>

constexpr uint32_t message_magic = 0xffac;

struct message_header {
  uint32_t magic;
  uint32_t len;
};

static void send_call_back(const std::error_code &error_code) {
  if (error_code) {
    std::cout << "send data with error code:" << error_code.message()
              << std::endl;
  } else {
    std::cout << "send data success" << std::endl;
  }
}

class message_receiver
    : public salt::header_body_assemble_notify<message_header> {
  salt::data_read_result
  packet_reserved(std::shared_ptr<salt::connection_handle> connection,
                  std::string raw_header_data, std::string body) override {
    auto &header = to_header(raw_header_data);
    header.magic = salt::byte_order::to_host(header.magic);
    header.len = salt::byte_order::to_host(header.len);
    std::cout << "get message, magic:" << std::hex << header.magic
              << ", content length:" << std::dec << header.len
              << " content:" << body << std::endl;
    return salt::data_read_result::success;
  }

  salt::data_read_result
  header_read_finish(std::shared_ptr<salt::connection_handle> connection,
                     const message_header &header) override {
    auto magic = header.magic;
    magic = salt::byte_order::to_host(magic);
    if (magic != message_magic) {
      std::cout << "magic error, expected:" << std::hex << message_magic
                << ", actual:" << std::hex << magic << std::endl;
      return salt::data_read_result::disconnect;
    }
    return salt::data_read_result::success;
  }

  void packet_read_error(const std::error_code &error_code,
                         const std::string &message) override {
    std::cerr << "packet read error, error code:" << error_code.value()
              << ", message:" << message << std::endl;
  }
};

static std::string make_packet(std::string message) {
  std::string result;
  auto message_size =
      static_cast<uint32_t>(sizeof(message_header::len) + message.size());
  result.reserve(message_size);
  auto magic_network_byte_order = salt::byte_order::to_network(message_magic);
  std::copy(reinterpret_cast<char *>(&magic_network_byte_order),
            reinterpret_cast<char *>(&magic_network_byte_order) +
                sizeof(decltype(magic_network_byte_order)),
            std::back_inserter(result));

  auto message_size_network_byte_order =
      salt::byte_order::to_network(message_size);
  std::copy(reinterpret_cast<char *>(&message_size_network_byte_order),
            reinterpret_cast<char *>(&message_size_network_byte_order) +
                sizeof(decltype(message_size_network_byte_order)),
            std::back_inserter(result));
  result += std::move(message);
  return result;
}

class tcp_client_notify : public salt::tcp_client_notify {
public:
  void connection_connected(const std::string &remote_addr,
                            uint16_t remote_port) override {
    std::cout << "connected to " << remote_addr << ":" << remote_port
              << std::endl;
  }

  void connection_disconnected(const std::error_code &error_code,
                               const std::string &remote_addr,
                               uint16_t remote_port) override {

    std::cout << "disconnected from " << remote_addr << ":" << remote_port
              << ", reason:" << error_code.message() << std::endl;
  }

  void connection_dropped(const std::string &remote_addr,
                          uint16_t remote_port) override {
    std::cout << "drop connection, remote addr:" << remote_addr << ":"
              << remote_port << std::endl;
  }
};

int main() {
  salt::tcp_client client;
  client.init(1);
  client.set_notify(std::make_unique<tcp_client_notify>());

  salt::connection_meta meta;
  meta.retry_when_connection_error = true;
  meta.retry_forever = true;
  meta.assemble_creator = [] {
    auto assemble =
        new salt::header_body_assemble<message_header, &message_header::len>();
    assemble->body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;
    assemble->set_notify(std::make_unique<message_receiver>());
    assemble->body_length_limit_ = 1024;
    return assemble;
  };

  client.connect("127.0.0.1", 2002, meta);

  while (true) {
    std::string line;
    std::getline(std::cin, line);

    util::string::in_place_trim(line);

    if (util::string::start_with(line, "\\quit")) {
      break;
    }

    auto broadcast = false;
    if (util::string::start_with(line, "\\broadcast")) {
      broadcast = true;
      line = line.substr(strlen("\\broadcast"));
      util::string::in_place_trim(line);
    }

    if (broadcast) {
      client.broadcast(make_packet(std::move(line)), send_call_back);
    } else {
      client.send("127.0.0.1", 2002, make_packet(std::move(line)),
                  send_call_back);
    }
  }

  client.disconnect("127.0.0.1", 2002);

  return 0;
}