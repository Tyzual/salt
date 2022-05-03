#pragma once

#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <system_error>

#include "asio.hpp"

#include "salt/asio_io_context_thread.h"
#include "salt/shared_asio_io_context_thread.h"
#include "salt/tcp_connection.h"

namespace salt {

#if 0
class tcp_client;

class tcp_client_start_argument {
public:
  tcp_client_start_argument &add_server(std::string address_v4, uint16_t port);

  tcp_client_start_argument &
  add_server(std::initializer_list<std::pair<std::string, uint16_t>> servers);

  inline tcp_client_start_argument &
  set_io_thread_count(uint32_t io_thread_count) {
    io_thread_count_ = io_thread_count;
    return *this;
  }

private:
  std::set<std::pair<std::string, uint16_t>> servers_;
  uint32_t io_thread_count_;

  friend class tcp_client;
};
#endif

class tcp_client {
public:
  tcp_client();
  ~tcp_client();
  void init(uint32_t transfer_thread_count);

  void connect(std::string address_v4, uint16_t port,
               std::function<void(const std::error_code &)> call_back);

  void disconnect(std::string address_v4, uint16_t port,
                  std::function<void(const std::error_code&)> call_back);

  void set_assemble_creator(
      std::function<base_packet_assemble *(void)> assemble_creator);

  void broadcast(uint32_t seq, std::string, tcp_connection::call_back);

  void stop();

private:
  void _connect(std::string address_v4, uint16_t port,
                std::function<void(const std::error_code &)> call_back);

  void _disconnect(std::string address_v4, uint16_t port,
                  std::function<void(const std::error_code&)> call_back);

private:
  asio::io_context transfor_io_context_;
  asio::executor_work_guard<asio::io_context::executor_type>
      transfor_io_context_work_guard_;
  asio_io_context_thread control_thread_;
  asio::ip::tcp::resolver resolver_;

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

  std::map<addr_v4, std::shared_ptr<tcp_connection>> connected_;
  std::map<addr_v4, std::shared_ptr<tcp_connection>> all_;
  std::function<base_packet_assemble *(void)> assemble_creator_{nullptr};
  std::vector<std::shared_ptr<shared_asio_io_context_thread>> io_threads_;
};

} // namespace salt