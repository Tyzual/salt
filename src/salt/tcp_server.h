#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "asio.hpp"

#include "salt/asio_io_context_thread.h"
#include "salt/shared_asio_io_context_thread.h"
#include "salt/tcp_connection.h"

namespace salt {

class TcpServer {
 public:
  TcpServer();
  ~TcpServer();
  void stop();
  bool init(uint16_t listen_port, uint32_t io_thread_cnt = 1);
  bool init(const std::string &listen_ip_v4, uint16_t listen_port, uint32_t io_thread_cnt = 1);
  void set_assemble_creator(std::function<BasePacketAssemble *(void)> assemble_creator);

  bool start();

  inline std::string get_listen_address() const { return listen_ip_.to_string(); }

  inline uint16_t get_listen_port() const { return listen_port_; }

 private:
  bool accept();

 private:
  uint16_t listen_port_{0};
  asio::ip::address_v4 listen_ip_{asio::ip::address_v4::any()};
  std::shared_ptr<asio::ip::tcp::acceptor> acceptor_{nullptr};
  asio::io_context transfor_io_context_;
  asio::executor_work_guard<asio::io_context::executor_type> transfor_io_context_work_guard_;
  AsioIoContextThread accept_thread_;
  std::vector<std::shared_ptr<SharedAsioIoContextThread>> io_threads_;
  std::function<BasePacketAssemble *(void)> assemble_creator_{nullptr};
};
}  // namespace salt