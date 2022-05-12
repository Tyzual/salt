#include "salt/core/tcp_connection.h"

#include <istream>
#include <ostream>

#include "salt/core/error.h"
#include "salt/core/log.h"
#include "salt/core/tcp_connection_handle.h"
#include "salt/util/call_back_wrapper.h"

namespace salt {

std::shared_ptr<tcp_connection> tcp_connection::create(
    asio::io_context &transfer_io_context,
    base_packet_assemble *packet_assemble,
    std::function<void(const std::string &remote_address, uint16_t remote_port,
                       const std::error_code &error_code)>
        read_notify_callback /* = nullptr */
) {
  auto connection = std::shared_ptr<tcp_connection>(new tcp_connection(
      transfer_io_context, packet_assemble, std::move(read_notify_callback)));
  if (connection) {
    connection->init();
  }
  return connection;
}

void tcp_connection::disconnect() {
  log_debug("socket %s:%u disconnect from %s:%u", local_address_.c_str(),
            local_port_, remote_address_.c_str(), remote_port_);
  std::error_code error_code;
  socket_.close(error_code);
  send_items_.clear();
  receive_buffer_.clear();
  receive_buffer_.resize(receive_buffer_max_size_);
  send_flag_.clear();
}

void tcp_connection::_send(
    std::string data, std::function<void(const std::error_code &)> call_back) {
  auto _this{shared_from_this()};
  asio::async_write(
      this->socket_, asio::buffer(data),
      asio::bind_executor(
          strand_, [this, _this, call_back](const std::error_code &err_code,
                                            std::size_t length) {
            call(call_back, err_code);
            if (this->send_items_.empty()) {
              this->send_flag_.clear();
              return;
            } else {
              auto item = this->send_items_.front();
              this->send_items_.pop_front();
              _send(std::move(item.first), std::move(item.second));
            }
          }));
}

void tcp_connection::send(
    std::string data, std::function<void(const std::error_code &)> call_back) {
  auto _this{shared_from_this()};
  transfer_io_context_.post(asio::bind_executor(
      strand_, [this, _this, data = std::move(data), call_back] {
        if (!send_flag_.test_and_set()) {
          // 没有未完成的写操作
          _send(data, std::move(call_back));
        } else {
          // 有未完成的写操作
          if (this->send_items_.size() > this->send_buffer_max_size_) {
            log_error("too many send items(%u), drop data",
                      this->send_buffer_max_size_);
            call(call_back, make_error_code(error_code::send_queue_full));
          } else {
            this->send_items_.push_back(
                std::make_pair(std::move(data), std::move(call_back)));
          }
          return;
        }
      }));
}

bool tcp_connection::read() {
  if (!packet_assemble_) {
    log_error("packet assemble is nullptr");
    return false;
  }
  auto _this{shared_from_this()};
  socket_.async_read_some(
      asio::buffer(receive_buffer_),
      [this, _this](const std::error_code &err_code, std::size_t data_length) {
        if (err_code && data_length <= 0) {
          log_error("read data from %s:%u error, reason:%s",
                    remote_address_.c_str(), remote_port_,
                    err_code.message().c_str());
          notify_connection_error(err_code);
          return;
        } else if (err_code) {
          log_error(
              "read data from %s:%u error, but remain %zu byte data, reason:%s",
              remote_address_.c_str(), remote_port_, data_length,
              err_code.message().c_str());
          notify_connection_error(err_code);
          return;
        }
        log_debug("receive %zu byte data", data_length);
        std::string data{this->receive_buffer_.begin(),
                         this->receive_buffer_.begin() + data_length};
        auto read_result = this->packet_assemble_->data_received(
            tcp_connection_handle::create(_this), std::move(data));
        if (read_result == data_read_result::disconnect) {
          log_error(
              "read data from %s:%u finish, packet assemble return disconnect",
              remote_address_.c_str(), remote_port_);
          this->disconnect();
          notify_connection_error(
              make_error_code(error_code::require_disconnecet));
          return;
        } else if (read_result == data_read_result::error) {
          log_error("read data from %s:%u error, but continue read",
                    remote_address_.c_str(), remote_port_);
        } else {
          log_debug("read data from %s:%u success, continue read",
                    remote_address_.c_str(), remote_port_);
        }
        this->read();
      });
  return true;
}

void tcp_connection::notify_connection_error(
    const std::error_code &error_code) {
  call(connection_notify_callback_, remote_address_, remote_port_, error_code);
}

void tcp_connection::handle_fail_connection(const std::error_code &error_code) {
  notify_connection_error(error_code);
}

} // namespace salt