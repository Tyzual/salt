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

class tcp_client_notify {
public:
  virtual ~tcp_client_notify() = default;

  virtual void connection_connected(const std::string &remote_addr,
                                    uint16_t remote_port) = 0;

  virtual void connection_disconnected(const std::error_code &error_code,
                                       const std::string &remote_addr,
                                       uint16_t remote_port) = 0;

  virtual void connection_dropped(const std::string &remote_addr,
                                  uint16_t remote_port) = 0;
};

struct connection_meta {
  bool retry_when_connection_error{true};
  bool retry_forever{false};
  uint32_t max_retry_cnt{3};
  uint32_t retry_interval_s{5};
  std::function<base_packet_assemble *(void)> assemble_creator;
};

class tcp_client {
public:
  tcp_client();
  ~tcp_client();

  tcp_client &set_transfer_thread_count(uint32_t transfer_thread_count);
  tcp_client &set_assemble_creator(
      std::function<base_packet_assemble *(void)> assemble_creator);
  tcp_client &set_notify(std::unique_ptr<tcp_client_notify> notify);

  [[deprecated("use set thansfer_thread_count instead")]] void
  init(uint32_t transfer_thread_count);

  void connect(std::string address_v4, uint16_t port,
               const connection_meta &meta);

  void connect(std::string address_v4, uint16_t port);

  void disconnect(std::string address_v4, uint16_t port);

  void broadcast(std::string data,
                 std::function<void(const std::error_code &)> call_back);

  void send(std::string address_v4, uint16_t port, std::string data,
            std::function<void(const std::error_code &)> call_back);

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