#pragma once

#include <deque>
#include <memory>
#include <string>
#include <system_error>

#include "asio.hpp"
#include "salt/packet_assemble.h"

namespace salt {

class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
public:
  static std::shared_ptr<tcp_connection>
  create(asio::io_context &transfer_io_context,
         base_packet_assemble *packet_assemble);

  inline asio::ip::tcp::socket &get_socket() { return socket_; }

  bool read();

  void send(std::string data,
            std::function<void(const std::error_code &)> call_back);

private:
  tcp_connection(asio::io_context &transfer_io_context,
                 base_packet_assemble *packet_assemble)
      : transfer_io_context_(transfer_io_context), socket_(transfer_io_context),
        packet_assemble_(packet_assemble),
        strand_(asio::make_strand(transfer_io_context)) {}

  void init() { receive_buffer_.resize(receive_buffer_max_size_); }

  void _send(std::string data,
             std::function<void(const std::error_code &)> call_back);
  void disconnect();

private:
  asio::io_context &transfer_io_context_;
  asio::ip::tcp::socket socket_;
  uint32_t send_buffer_max_size_{256};
  std::deque<
      std::pair<std::string, std::function<void(const std::error_code &)>>>
      send_items_;
  uint32_t receive_buffer_max_size_{1024};
  std::string receive_buffer_;
  std::unique_ptr<base_packet_assemble> packet_assemble_{nullptr};
  asio::strand<asio::io_context::executor_type> strand_;
  std::atomic_flag write_flag_;
};

} // namespace salt