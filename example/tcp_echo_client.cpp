#include <iostream>
#include <string>

#include "salt/packet_assemble.h"
#include "salt/tcp_client.h"
#include "salt/version.h"

#include "util/string_util.h"

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
    std::cout << "received data: " << s.c_str() << std::endl;
    return salt::data_read_result::success;
  }
};

int main() {

  salt::tcp_client client;
  client.init(1);
  client.set_assemble_creator([] { return new echo_packet_assemble(); });

  salt::connection_meta meta;
  meta.retry_when_connection_error = true;
  meta.retry_forever = false;
  meta.max_retry_cnt = 10;
  meta.retry_interval_s = 1;

  client.connect(
      "127.0.0.1", 2002, meta, [](const std::error_code &error_code) {
        if (error_code) {
          std::cout << "connect to 127.0.0.1:2002 error, reason:"
                    << error_code.message() << std::endl;
        } else {
          std::cout << "connect to 127.0.0.1:2002 success" << std::endl;
        }
      });

  while (true) {
    std::string line;
    std::getline(std::cin, line);

    util::string::in_place_trim(line);

    if (util::string::start_with(line, "\\quit")) {
      return 0;
    }

    auto broadcast = false;
    if (util::string::start_with(line, "\\broadcast")) {
      broadcast = true;
      line = line.substr(strlen("\\broadcast"));
      util::string::in_place_trim(line);
    }

    if (broadcast) {
      client.broadcast(line, send_call_back);
    } else {
      client.send("127.0.0.1", 2002, line, send_call_back);
    }
  }

  client.disconnect("127.0.0.1", 2002, [](const std::error_code &error_code) {
    if (error_code) {
      std::cout << "disconnect to 127.0.0.1:2002 error, reason:"
                << error_code.message() << std::endl;
    } else {
      std::cout << "disconnect to 127.0.0.1:2002 success" << std::endl;
    }
  });

  return 0;
}