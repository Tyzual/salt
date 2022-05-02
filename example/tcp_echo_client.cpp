#include <iostream>

#include "salt/packet_assemble.h"
#include "salt/tcp_client.h"
#include "salt/version.h"

static void send_call_back(uint32_t seq, const std::error_code &error_code) {
  std::cout << "send data seq:" << seq
            << ", with error code:" << error_code.message() << std::endl;
}

class echo_packet_assemble : public salt::base_packet_assemble {
public:
  salt::data_read_result
  data_received(std::shared_ptr<salt::connection_handle> connection,
                std::string s) override {
    static uint32_t seq = 0;
    connection->send(++seq, std::move(s), send_call_back);

    return salt::data_read_result::success;
  }
};

int main() {

  salt::tcp_client client;
  client.init(1);
  client.set_assemble_creator([] { return new echo_packet_assemble(); });
  client.connect("127.0.0.1", 2002, [](const std::error_code &error_code) {
    if (error_code) {
      std::cout << "connect to 127.0.0.1:2002 error, reason:"
                << error_code.message() << std::endl;
      // std::exit(1);
    } else {
      std::cout << "connect to 127.0.0.1:2002 success" << std::endl;
    }
  });

  std::cin.get();
  client.disconnect("127.0.0.1", 2002, [](const std::error_code &error_code) {
    if (error_code) {
      std::cout << "disconnect to 127.0.0.1:2002 error, reason:"
                << error_code.message() << std::endl;
    } else {
      std::cout << "disconnect to 127.0.0.1:2002 success" << std::endl;
    }
  });

  std::cin.get();

  return 0;
}