#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "asio.hpp"

#include "salt/core/asio_io_context_thread.h"
#include "salt/core/shared_asio_io_context_thread.h"
#include "salt/core/tcp_connection.h"

namespace salt {

/**
 * @brief tcp 服务器，你可以通过此类来启动一个 tcp 服务器。
 *
 */
class tcp_server {
public:
  tcp_server();
  ~tcp_server();

  /**
   * @deprecated 已废弃，使用
   *            set_listen_ip_v4
   *            set_listen_port
   *            set_transfer_thread_count
   *            方法替代
   *
   * @brief 初始化服务器参数
   *
   * @param listen_port 监听端口
   * @param io_thread_cnt 后台传输线程个数
   * @return true 初始化成功
   * @return false 初始化失败
   */
  [[deprecated("use set_io_thread_count, set_listen_ip_v4, set_listen_port "
               "instead")]] bool
  init(uint16_t listen_port, uint32_t io_thread_cnt = 1);

  /**
   * @deprecated 已废弃，使用 set_listen_ip_v4 set_listen_port
   * set_transfer_thread_count 方法替代
   *
   * @brief 初始化服务器参数
   *
   * @param listen_ip_v4 监听的ipv4地址
   * @param listen_port 监听端口
   * @param io_thread_cnt 后台传输线程个数
   * @return true 初始化成功
   * @return false 初始化失败
   */
  [[deprecated("use set_io_thread_count, set_listen_ip_v4, set_listen_port "
               "instead")]] bool
  init(const std::string &listen_ip_v4, uint16_t listen_port,
       uint32_t io_thread_cnt = 1);

  /**
   * @brief 设置需要监听的ip地址，如果不调用此方法，则监听"0.0.0.0"
   *        如果传入非法ip地址，次方法会throw
   *
   * @param listen_ip_v4 需要监听的ip地址
   * @return tcp_server& tcp_server 自己
   * @throw std::exception 传入非法ip地址时，此方法会throw
   */
  tcp_server &set_listen_ip_v4(const std::string &listen_ip_v4);

  /**
   * @brief 设置需要监听的端口
   *
   * @param listen_port 端口
   * @return tcp_server& tcp_server 自己
   */
  tcp_server &set_listen_port(uint16_t listen_port);

  /**
   * @brief 设置后台传输线程的个数
   *
   * @param transfer_thread_count 后台传输线程个数
   * @return tcp_server& tcp_server 自己
   */
  tcp_server &set_transfer_thread_count(uint32_t transfer_thread_count);

  /**
   * @brief
   * 设置拆包器工厂函数。收到新链接时，框架会调用这个函数为链接创建一个拆包器用于解决粘包问题，详细说明请看
   * base_packet_assemble 说明
   *
   * @param assemble_creator 拆包器工厂函数
   * @return tcp_server& tcp_server 自己
   */
  tcp_server &set_assemble_creator(
      std::function<base_packet_assemble *(void)> assemble_creator);

  /**
   * @brief 启动服务器
   *
   * @return std::error_code 启动结果，salt::error_code::success 代表启动成功
   */
  std::error_code start();

  /**
   * @brief
   * 停止服务器，调用以后。服务器停止以后，如果需要重启服务器，请创建一个新的服务器实例，不要再已经停止的服务器上调用start
   *
   */
  void stop();

  /**
   * @brief 获取监听的地址
   *
   * @return std::string 监听地址
   */
  inline std::string get_listen_address() const {
    return listen_ip_.to_string();
  }

  /**
   * @brief 获取监听的端口
   *
   * @return uint16_t 监听端口
   */
  inline uint16_t get_listen_port() const { return listen_port_; }

private:
  std::error_code accept();

private:
  uint16_t listen_port_{0};
  asio::ip::address_v4 listen_ip_{asio::ip::address_v4::any()};
  std::shared_ptr<asio::ip::tcp::acceptor> acceptor_{nullptr};
  asio::io_context transfer_io_context_;
  asio::executor_work_guard<asio::io_context::executor_type>
      transfer_io_context_work_guard_;
  asio_io_context_thread accept_thread_;
  std::vector<std::shared_ptr<shared_asio_io_context_thread>> io_threads_;
  std::function<base_packet_assemble *(void)> assemble_creator_{nullptr};
};
} // namespace salt