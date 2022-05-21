#include <iostream>

#include "salt/core/tcp_server.h"
#include "salt/packet_assemble/packet_assemble.h"
#include "salt/version.h"

/**
 * 发送完成后的回调
 *
 */
static void send_call_back(const std::error_code &error_code) {
  if (error_code) {
    std::cout << "send data with error code:" << error_code.message()
              << std::endl;
  } else {
    std::cout << "send data success" << std::endl;
  }
}

/**
 * echo 拆包器，仅仅将收到的数据发送回去
 *
 */
class echo_packet_assemble : public salt::base_packet_assemble {
public:
  /**
   * 服务器接收到数据时会调用到这个方法，这里将收到的数据又发了回去
   */
  salt::data_read_result
  data_received(std::shared_ptr<salt::connection_handle> connection,
                std::string s) override {
    connection->send(std::move(s), send_call_back);

    return salt::data_read_result::success;
  }
};

/**
 * tcp 服务器端的示例
 *
 */
int main() {

  // 创建服务器，设置监听端口，设置拆包器工厂
  salt::tcp_server server;
  server.set_listen_port(2002).set_assemble_creator(
      [] { return new echo_packet_assemble(); });

  // 启动服务器
  auto err_code = server.start();
  if (!err_code) {
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