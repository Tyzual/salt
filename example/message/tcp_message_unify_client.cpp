#include "salt/core/tcp_client.h"
#include "salt/packet_assemble/header_body_unify_assemble.h"
#include "salt/util/byte_order.h"

#include "util/string_util.h"

#include <algorithm>
#include <iostream>
#include <string_view>

constexpr uint32_t message_magic = 0xffac;

// 包头
struct message_header {
  // magic 用于校验
  uint32_t magic;

  // 包长度
  uint32_t len;

  // 随便填了点东西，用来表明长度字段不一定非要是头部的最后一个字段
  char what_ever[8];
};

/**
 * 发送结果回调
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
 * 拆完包后的监听器
 *
 */
class message_receiver
    : public salt::header_body_unify_assemble_notify<message_header> {

  /**
   * 拆包器成功拆完一个包以后会调用这个接口
   *
   */
  salt::data_read_result
  packet_reserved(std::shared_ptr<salt::connection_handle> connection,
                  std::string packet) override {
    auto packet_view = std::string_view(packet);
    auto &header = to_header(packet);
    header.magic = salt::byte_order::to_host(header.magic);
    header.len = salt::byte_order::to_host(header.len);
    std::cout << "get message, magic:" << std::hex << header.magic
              << ", content length:" << std::dec << header.len
              << " content:" << packet_view.substr(sizeof(header)) << std::endl;
    return salt::data_read_result::success;
  }

  /**
   * 拆包器解析完包头以后调用的接口
   */
  salt::data_read_result
  header_read_finish(std::shared_ptr<salt::connection_handle> connection,
                     const std::string &raw_header_data) override {
    const auto &raw_header = to_header(raw_header_data);

    /**
     * 在这里象征性的演示了一下如何验证magic
     *
     */
    auto magic = salt::byte_order::to_host(raw_header.magic);
    if (magic != message_magic) {
      std::cout << "magic error, expected:" << std::hex << message_magic
                << ", actual:" << std::hex << magic << std::endl;
      return salt::data_read_result::disconnect;
    }
    return salt::data_read_result::success;
  }

  /**
   * 读取包错误，通常是在包长异常时调用
   */
  void packet_read_error(const std::error_code &error_code,
                         const std::string &message) override {
    std::cerr << "packet read error, error code:" << error_code.value()
              << ", message:" << message << std::endl;
  }
};

/**
 * 创建消息包
 */
static std::string make_packet(std::string message) {
  std::string result;

  // 首先计算包大小，由于我们解包时候设置的是with_length_field
  // 所以body长度需要在实际长度上增加4，这里用sizeof来计算了
  auto message_size =
      static_cast<uint32_t>(sizeof(message_header::len) + message.size());
  result.reserve(message_size);

  // 随便设置一个magic，并且转成网络字节序
  auto magic_network_byte_order = salt::byte_order::to_network(message_magic);

  // 将magic写入数据包
  std::copy(reinterpret_cast<char *>(&magic_network_byte_order),
            reinterpret_cast<char *>(&magic_network_byte_order) +
                sizeof(decltype(magic_network_byte_order)),
            std::back_inserter(result));

  // 同理，将包长写入数据包
  auto message_size_network_byte_order =
      salt::byte_order::to_network(message_size);
  std::copy(reinterpret_cast<char *>(&message_size_network_byte_order),
            reinterpret_cast<char *>(&message_size_network_byte_order) +
                sizeof(decltype(message_size_network_byte_order)),
            std::back_inserter(result));

  // 写入what_ever字段
  decltype(message_header::what_ever) what_ever{0};
  std::copy(std::begin(what_ever), std::end(what_ever),
            std::back_inserter(result));

  // 最后写入正文
  result += std::move(message);
  return result;
}

/**
 * 客户端的监听，链接发生变动时会调到这里来
 *
 */
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

/**
 * 一个使用包头，包内容拆包器的示例
 * 可以配合tcp_echo_server使用
 */
int main() {

  // 创建客户端，设置传输线程以及监听对象
  salt::tcp_client client;
  client.set_transfer_thread_count(1).set_notify(
      std::make_unique<tcp_client_notify>());

  // 配置链接
  salt::connection_meta meta;
  meta.retry_when_connection_error = true;
  meta.retry_forever = true;

  // 在这里配置了一个链接级别的拆包器工厂
  meta.assemble_creator = [] {
    // 创建基于包头，包内容的拆包器
    // 包头结构为 message_header
    // 其中 message_header::len 字段用于表示包内容的长度
    auto assemble =
        new salt::header_body_unify_assemble<message_header,
                                                    &message_header::len>();

    // 设置包长度的计算方式，这里设置为包含了包长度字段自生的长度。
    // 所以，Salt
    // 在解析出长度时，会将解析出的长度减去sizeof(message_header::len)即4个byte作为包内容长度
    // 对应的，在发送数据时，也需要将message_header::len字段设置为实际内容长度+4
    assemble->body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;

    // 设置拆包完成的监听器，当拆包完成或者异常时，会调用这个对象的对应方法
    assemble->set_notify(std::make_unique<message_receiver>());

    // 设置最大包限制，如果包内容长度超过这个限制，则断开链接。
    // 这里为了演示设置了只能发15个byte
    // 设置为0则不限制
    assemble->body_length_limit_ = 15;
    return assemble;
  };

  // 链接客户端
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

    // 注意这里发送时用了make_packet函数来将发送内容封装为数据包
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