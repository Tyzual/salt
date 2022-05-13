#include <iostream>

#include "salt/core/tcp_server.h"
#include "salt/packet_assemble/packet_assemble.h"
#include "salt/version.h"

static void send_call_back(const std::error_code &error_code) {
  if (error_code) {
    std::cout << "send data with error code:" << error_code.message()
              << std::endl;
  } else {
    std::cout << "send data success" << std::endl;
  }
}

class echo_packet_assemble : public salt::base_packet_assemble {
public:
  salt::data_read_result
  data_received(std::shared_ptr<salt::connection_handle> connection,
                std::string s) override {
    connection->send(std::move(s), send_call_back);

    return salt::data_read_result::success;
  }
};

int main() {
  salt::tcp_server server;
  server.set_assemble_creator([] { return new echo_packet_assemble(); });
  server.init(2002);
  if (server.start()) {
    std::cout << "server start success, salt version:" << salt::version
              << std::endl;
    std::cin.get();
    return 0;
  } else {
    std::cerr << "server start error, salt version:" << salt::version
              << std::endl;
    return 1;
  }
  return 0;
}