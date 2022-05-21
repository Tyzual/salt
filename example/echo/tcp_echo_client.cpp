#include <chrono>
#include <iostream>
#include <string>

#include "salt/core/tcp_client.h"
#include "salt/packet_assemble/packet_assemble.h"
#include "salt/version.h"

#include "util/string_util.h"

// 发送的回调函数，可以从这里知道是否发送成功
static void send_call_back(const std::error_code &error_code) {
  if (error_code) {
    std::cout << "send data with error code:" << error_code.message()
              << std::endl;
  } else {
    std::cout << "send data success" << std::endl;
  }
}

/**
 * 一个packet assemble例子，仅仅将收到的内容打印出来
 */
class echo_packet_assemble : public salt::base_packet_assemble {
public:
  salt::data_read_result
  data_received(std::shared_ptr<salt::connection_handle> connection,
                std::string s) override {
    std::cout << "received data: " << s.c_str() << std::endl;
    return salt::data_read_result::success;
  }
};

/**
 * 客户端链接发生改变时会调这里。
 */
class tcp_client_notify : public salt::tcp_client_notify {
public:

  // 成功链接
  void connection_connected(const std::string &remote_addr,
                            uint16_t remote_port) override {
    std::cout << "connected to " << remote_addr << ":" << remote_port
              << std::endl;
  }

  // 链接失败或者断连，是否重试需要看调用connect时候的meta参数配置
  void connection_disconnected(const std::error_code &error_code,
                               const std::string &remote_addr,
                               uint16_t remote_port) override {

    std::cout << "disconnected from " << remote_addr << ":" << remote_port
              << ", reason:" << error_code.message() << std::endl;
  }

  // 链接断开，并且不再尝试重连
  void connection_dropped(const std::string &remote_addr,
                          uint16_t remote_port) override {
    std::cout << "drop connection, remote addr:" << remote_addr << ":"
              << remote_port << std::endl;
  }
};


/**
 * tcp 客户端的示例
 */
int main() {


  // 创建客户端，设置拆包器工厂以及监听对象
  salt::tcp_client client;
  client.set_transfer_thread_count(2)
      .set_assemble_creator([] { return new echo_packet_assemble(); })
      .set_notify(std::make_unique<tcp_client_notify>());

  // 配置重连信息
  salt::connection_meta meta;
  meta.retry_when_connection_error = true;
  meta.retry_forever = true;
  meta.max_retry_cnt = 10;
  meta.retry_interval_s = 0;

  // 链接服务器
  client.connect("127.0.0.1", 2002, meta);


  // 读取用户输入，发送消息
  // 输入 \quit 退出客户端
  // 输入 \broadcast <内容> 给所有连接的服务器发送数据
  // 输入其它内容给 127.0.0.1:2002 发送内容
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

    // 发送内容的代码主要看这里
    if (broadcast) {
      client.broadcast(line, send_call_back);
    } else {
      client.send("127.0.0.1", 2002, line, send_call_back);
    }
  }

  client.disconnect("127.0.0.1", 2002);

  return 0;
}