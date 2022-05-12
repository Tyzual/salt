#pragma once

#include <deque>
#include <memory>
#include <string>
#include <system_error>

#include "asio.hpp"

#include "salt/core/log.h"
#include "salt/packet_assemble/packet_assemble.h"

namespace salt {

class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
public:
  static std::shared_ptr<tcp_connection>
  create(asio::io_context &transfer_io_context,
         base_packet_assemble *packet_assemble,
         std::function<void(const std::string &remote_address,
                            uint16_t remote_port,
                            const std::error_code &error_code)>
             read_notify_callback = nullptr);

  inline asio::ip::tcp::socket &get_socket() { return socket_; }

  bool read();

  void send(std::string data,
            std::function<void(const std::error_code &)> call_back);

  void disconnect();

  ~tcp_connection() {
    log_debug("~tcp_connection:%p", this);
    disconnect();
  }

  inline void set_remote_address(std::string remote_address) {
    remote_address_ = std::move(remote_address);
  }

  inline const std::string &get_remote_address() const {
    return remote_address_;
  }

  inline void set_remote_port(uint16_t remote_port) {
    remote_port_ = remote_port;
  }

  inline uint16_t get_remote_port() const { return remote_port_; }

  inline void set_local_address(std::string local_address) {
    local_address_ = std::move(local_address);
  }

  inline const std::string &get_local_address() const { return local_address_; }

  inline void set_local_port(uint16_t local_port) { local_port_ = local_port; }

  inline uint16_t get_local_port() const { return local_port_; }

  void handle_fail_connection(const std::error_code &error_code);

private:
  tcp_connection(asio::io_context &transfer_io_context,
                 base_packet_assemble *packet_assemble,
                 std::function<void(const std::string &addr, uint16_t port,
                                    const std::error_code &error_code)>
                     connection_notify_callback)
      : transfer_io_context_(transfer_io_context), socket_(transfer_io_context),
        packet_assemble_(packet_assemble),
        strand_(asio::make_strand(transfer_io_context)),
        connection_notify_callback_(std::move(connection_notify_callback)) {
    log_debug("create tcp_conection:%p", this);
  }

  void init() { receive_buffer_.resize(receive_buffer_max_size_); }

  void _send(std::string data,
             std::function<void(const std::error_code &)> call_back);

  void notify_connection_error(const std::error_code &error_code);

private:
  asio::io_context &transfer_io_context_;
  asio::ip::tcp::socket socket_;
  uint32_t send_buffer_max_size_{256};
  std::deque<std::pair<std::string /* data */,
                       std::function<void(const std::error_code &)>>>
      send_items_;
  uint32_t receive_buffer_max_size_{1024};
  std::string receive_buffer_;
  std::unique_ptr<base_packet_assemble> packet_assemble_{nullptr};
  asio::strand<asio::io_context::executor_type> strand_;
  std::atomic_flag send_flag_{false};
  std::string remote_address_;
  uint16_t remote_port_{0};
  std::string local_address_;
  uint16_t local_port_{0};
  std::function<void(const std::string &remote_address, uint16_t remote_port,
                     const std::error_code &error_code)>
      connection_notify_callback_;
};

} // namespace salt