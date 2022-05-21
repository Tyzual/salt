#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "asio.hpp"

#include "salt/core/asio_io_context_thread.h"
#include "salt/core/shared_asio_io_context_thread.h"
#include "salt/core/tcp_connection.h"

namespace salt {

class tcp_server {
public:
  tcp_server();
  ~tcp_server();

  void stop();

  [[deprecated("use set_io_thread_count, set_listen_ip_v4, set_listen_port "
               "instead")]] bool
  init(uint16_t listen_port, uint32_t io_thread_cnt = 1);

  [[deprecated("use set_io_thread_count, set_listen_ip_v4, set_listen_port "
               "instead")]] bool
  init(const std::string &listen_ip_v4, uint16_t listen_port,
       uint32_t io_thread_cnt = 1);

  tcp_server &set_listen_ip_v4(const std::string &listen_ip_v4);
  tcp_server &set_listen_port(uint16_t listen_port);
  tcp_server &set_transfer_thread_count(uint32_t transfer_thread_count);
  tcp_server &set_assemble_creator(
      std::function<base_packet_assemble *(void)> assemble_creator);

  std::error_code start();

  inline std::string get_listen_address() const {
    return listen_ip_.to_string();
  }

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