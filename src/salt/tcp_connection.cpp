#include "salt/tcp_connection.h"

#include <istream>
#include <ostream>

#include "salt/error.h"
#include "salt/log.h"
#include "salt/tcp_connection_handle.h"

namespace salt {

std::shared_ptr<tcp_connection>
tcp_connection::create(asio::io_context &transfer_io_context,
                       base_packet_assemble *packet_assemble) {
  auto connection = std::shared_ptr<tcp_connection>(
      new tcp_connection(transfer_io_context, packet_assemble));
  if (connection) {
    connection->init();
  }
  return connection;
}

void tcp_connection::disconnect() {
  if (socket_.is_open()) {
    log_debug("socket %s:%u disconnect from %s:%u",
              socket_.local_endpoint().address().to_string().c_str(),
              socket_.local_endpoint().port(),
              socket_.remote_endpoint().address().to_string().c_str(),
              socket_.remote_endpoint().port());
  }
  socket_.close();
  send_items_.clear();
  receive_buffer_.clear();
  receive_buffer_.resize(receive_buffer_max_size_);
  write_flag_.clear();
}

void tcp_connection::_send(
    uint32_t seq, std::string data,
    std::function<void(uint32_t seq, const std::error_code &)> call_back) {
  auto _this{shared_from_this()};
  asio::async_write(
      this->socket_, asio::buffer(data),
      asio::bind_executor(
          strand_, [this, _this, call_back,
                    seq](const std::error_code &err_code, std::size_t length) {
            call_back(seq, err_code);
            if (this->send_items_.empty()) {
              this->write_flag_.clear();
              return;
            } else {
              auto item = this->send_items_.front();
              this->send_items_.pop_front();
              this->_send(std::get<0>(item), std::move(std::get<1>(item)),
                          std::get<2>(item));
            }
          }));
}

void tcp_connection::send(
    uint32_t seq, std::string data,
    std::function<void(uint32_t seq, const std::error_code &)> call_back) {
  auto _this{shared_from_this()};
  transfer_io_context_.post(asio::bind_executor(
      strand_, [this, _this, data = std::move(data), call_back, seq] {
        if (!write_flag_.test_and_set()) {
          // 没有未完成的写操作
          _send(seq, data, call_back);
        } else {
          // 有未完成的写操作
          if (this->send_items_.size() > this->send_buffer_max_size_) {
            log_error("too many send items(%u), drop data",
                      this->send_buffer_max_size_);
            call_back(seq, make_error_code(error_code::send_queue_full));
          } else {
            this->send_items_.push_back(
                std::make_tuple(seq, std::move(data), call_back));
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
          log_error(
              "read data from %s:%u error, reason:%s",
              this->socket_.remote_endpoint().address().to_string().c_str(),
              this->socket_.remote_endpoint().port(),
              err_code.message().c_str());
          return;
        } else if (err_code) {
          log_error(
              "read data from %s:%u error, but remain %zu byte data, reason:%s",
              this->socket_.remote_endpoint().address().to_string().c_str(),
              this->socket_.remote_endpoint().port(), data_length,
              err_code.message().c_str());
          return;
        }
        log_debug("receive %zu byte data", data_length);
        std::string data{this->receive_buffer_.begin(),
                         this->receive_buffer_.begin() + data_length};
        auto read_result = this->packet_assemble_->data_received(
            tcp_connection_handle::create(_this), std::move(data));
        if (read_result == data_read_result::disconnect) {
          log_error(
              "read data from %s:%u error, disconnect",
              this->socket_.remote_endpoint().address().to_string().c_str(),
              this->socket_.remote_endpoint().port());
          this->disconnect();
          return;
        } else if (read_result == data_read_result::error) {
          log_error(
              "read data from %s:%u error, but continue read",
              this->socket_.remote_endpoint().address().to_string().c_str(),
              this->socket_.remote_endpoint().port());
        } else {
          log_debug(
              "read data from %s:%u success, continue read",
              this->socket_.remote_endpoint().address().to_string().c_str(),
              this->socket_.remote_endpoint().port());
        }
        this->read();
      });
  return true;
}

} // namespace salt