#pragma once

#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <system_error>

#include "asio.hpp"

#include "salt/core/asio_io_context_thread.h"
#include "salt/core/shared_asio_io_context_thread.h"
#include "salt/core/tcp_connection.h"

/**
 * @brief Salt 主命名空间
 *
 */
namespace salt {

/**
 * @brief 客户端的监听对象接口
 *
 */
class tcp_client_notify {
public:
  virtual ~tcp_client_notify() = default;

  /**
   * @brief 当客户端成功连接到一台服务器时，会调用这个接口
   *
   * @param remote_addr 服务器地址
   * @param remote_port 服务器端口
   */
  virtual void connection_connected(const std::string &remote_addr,
                                    uint16_t remote_port) = 0;

  /**
   * @brief 当客户端与服务器端的链接终端时，会调用这个接口
   *
   * @param error_code 中断的原因
   * @param remote_addr 中断的远端服务器地址
   * @param remote_port 中断的远端服务器端口
   */
  virtual void connection_disconnected(const std::error_code &error_code,
                                       const std::string &remote_addr,
                                       uint16_t remote_port) = 0;

  /**
   * @brief
   * 当客户端与服务器端的链接中断，并且客户端不再尝试重连时，会调用这个接口
   *
   * @param remote_addr 服务器地址
   * @param remote_port 服务器端口
   */
  virtual void connection_dropped(const std::string &remote_addr,
                                  uint16_t remote_port) = 0;
};

/**
 * @brief 连接到服务器时的可选额外信息
 *
 */
struct connection_meta {
  /**
   * @brief 链接异常中断或者链接失败时，是否重试
   *
   */
  bool retry_when_connection_error{true};

  /**
   * @brief 是否一直重试
   *
   */
  bool retry_forever{false};

  /**
   * @brief 重试的最大次数，这个属性仅在 retry_forever 为 false 时生效
   *
   */
  uint32_t max_retry_cnt{3};

  /**
   * @brief
   * 链接中断或者链接失败时，尝试重试前的等待时间，如果设置为0，则立刻重试
   *
   */
  uint32_t retry_interval_s{5};

  /**
   * @brief
   * 链接级别的拆包器工厂函数，当链接到服务器时，如果设置了这个属性，则使用这个函数来创建拆包器。否则使用
   * tcp_client 中全局工厂函数创建拆包器
   *
   */
  std::function<base_packet_assemble *(void)> assemble_creator;
};

/**
 * @brief tcp 客户端，可以通过此类来连接一个 tcp 服务器
 *
 */
class tcp_client {
public:
  tcp_client();
  ~tcp_client();

  /**
   * @brief 设置后台传输线程个数
   *
   * @param transfer_thread_count 后台传输线程个数
   * @return tcp_client& tcp_client 自己
   */
  tcp_client &set_transfer_thread_count(uint32_t transfer_thread_count);

  /**
   * @brief
   * 设置拆包器工厂函数。连接到服务器时，框架会调用这个函数为链接创建一个拆包器用于解决粘包问题，详细说明请看
   * base_packet_assemble 说明
   *
   * @param assemble_creator 拆包器工厂函数
   * @return tcp_client& tcp_client 自己
   */
  tcp_client &set_assemble_creator(
      std::function<base_packet_assemble *(void)> assemble_creator);

  /**
   * @brief
   * 设置客户端的监听对象，当客户端的链接发生变动（链接建立成功，链接终端等等）时，监听对象会得到通知
   *
   * @param notify 监听对象
   * @return tcp_client& tcp_client 自己
   */
  tcp_client &set_notify(std::unique_ptr<tcp_client_notify> notify);

  /**
   * @deprecated 废弃，使用 set_transfer_thread_count 代替
   * @brief 初始化客户端，设置后台线程个数
   *
   * @param transfer_thread_count 后台传输线程个数
   */
  [[deprecated("use set thansfer_thread_count instead")]] void
  init(uint32_t transfer_thread_count);

  /**
   * @brief 连接到服务器
   *
   * @param address_v4 服务器 ip 地址
   * @param port 服务器端口
   * @param meta 链接的额外信息，用于配置链接断开或者连接失败时的行为
   */
  void connect(std::string address_v4, uint16_t port,
               const connection_meta &meta);

  /**
   * @brief 连接到服务器，链接断开时不自动重连
   *
   * @param address_v4 服务器 ip 地址(或者域名)
   * @param port 服务器端口
   */
  void connect(std::string address_v4, uint16_t port);

  /**
   * @brief 断开链接
   *
   * @param address_v4 服务器 ip 地址(或者域名)，需要与connect时传入的一致
   * @param port 服务器端口
   */
  void disconnect(std::string address_v4, uint16_t port);

  /**
   * @brief 向所有已经连接的服务器发送数据
   *
   * @param data 需要发送的数据
   * @param call_back
   * 发送数据完成的回调，一次发送有且仅有一次回调调用，可以从error_code参数获取是否发送成功
   */
  void broadcast(std::string data,
                 std::function<void(const std::error_code &)> call_back);

  /**
   * @brief 向一台已经连接的服务器发送数据
   *
   * @param address_v4 服务器 ip 地址
   * @param port 服务器端口
   * @param data 需要发送的数据
   * @param call_back
   * 发送数据完成的回调，一次发送有且仅有一次回调调用，可以从error_code参数获取是否发送成功
   */
  void send(std::string address_v4, uint16_t port, std::string data,
            std::function<void(const std::error_code &)> call_back);

  /**
   * @brief
   * 停止客户端，调用以后客户端会断开所有链接。客户端停止以后，如果需要重新链接，请创建一个新的客户端实例，不要再已经停止的客户端上调用connect
   *
   */
  void stop();

private:
  void _connect(std::string address_v4, uint16_t port);

  void _disconnect(std::string address_v4, uint16_t port);

  void _send(std::string address_v4, uint16_t port, std::string data,
             std::function<void(const std::error_code &)> call_back);

  void handle_connection_error(const std::string &remote_address,
                               uint16_t remote_port,
                               const std::error_code &error_code);

  void notify_connected(const std::string &remote_addr, uint16_t remote_port);

  void notify_disconnected(const std::error_code &error_code,
                           const std::string &remote_addr,
                           uint16_t remote_port);

  void notify_dropped(const std::string &remote_addr, uint16_t remote_port);

private:
  struct addr_v4 {
    std::string host;
    uint16_t port;

    bool operator<(const addr_v4 &rhs) const {
      if (host != rhs.host) {
        return host < rhs.host;
      }
      return port < rhs.port;
    }
  };

  struct connection_meta_impl {
    connection_meta meta_;
    uint32_t current_retry_cnt_{0};
  };

  std::map<addr_v4, std::shared_ptr<tcp_connection>> connected_;
  std::map<addr_v4, std::shared_ptr<tcp_connection>> all_;
  std::map<addr_v4, connection_meta_impl> connection_metas_;
  std::function<base_packet_assemble *(void)> assemble_creator_{nullptr};
  std::unique_ptr<tcp_client_notify> notify_{nullptr};

private:
  asio::io_context transfer_io_context_;
  asio::executor_work_guard<asio::io_context::executor_type>
      transfer_io_context_work_guard_;
  asio_io_context_thread control_thread_;
  asio::ip::tcp::resolver resolver_;
  std::vector<std::shared_ptr<shared_asio_io_context_thread>> io_threads_;
};

} // namespace salt