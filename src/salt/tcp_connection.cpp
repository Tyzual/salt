#include "salt/tcp_connection.h"

#include <istream>
#include <ostream>

#include "salt/error.h"
#include "salt/log.h"
#include "salt/tcp_connection_handle.h"

namespace salt {

std::shared_ptr<TcpConnection> TcpConnection::create(asio::io_context& transfer_io_context,
                                                     BasePacketAssemble* packet_assemble) {
  auto connection = std::shared_ptr<TcpConnection>(new TcpConnection(transfer_io_context, packet_assemble));
  if (connection) {
    connection->init();
  }
  return connection;
}

void TcpConnection::disconnect() {
  socket_.close();
  send_items_.clear();
  receive_buffer_.clear();
  receive_buffer_.resize(receive_buffer_max_size_);
  write_flag_.clear();
}

void TcpConnection::_send(std::string data, std::function<void(const std::error_code&)> call_back) {
  auto _this{shared_from_this()};
  asio::async_write(
      this->socket_, asio::buffer(data),
      asio::bind_executor(strand_, [this, _this, call_back](const std::error_code& err_code, std::size_t length) {
        call_back(err_code);
        if (this->send_items_.empty()) {
          this->write_flag_.clear();
          return;
        } else {
          auto item = this->send_items_.front();
          this->send_items_.pop_front();
          this->_send(std::move(item.first), item.second);
        }
      }));
}

void TcpConnection::send(std::string data, std::function<void(const std::error_code&)> call_back) {
  auto _this{shared_from_this()};
  transfer_io_context_.post(asio::bind_executor(strand_, [this, _this, data = std::move(data), call_back] {
    if (!write_flag_.test_and_set()) {
      // 没有未完成的写操作
      _send(data, call_back);
    } else {
      // 有未完成的写操作
      if (this->send_items_.size() > this->send_buffer_max_size_) {
        LogError("too many send items(%u), drop data", this->send_buffer_max_size_);
        call_back(make_error_code(ErrorCode::kSendQueueFull));
      } else {
        this->send_items_.push_back(std::make_pair(std::move(data), call_back));
      }
      return;
    }
  }));
}

bool TcpConnection::read() {
  if (!packet_assemble_) {
    LogError("packet assemble is nullptr");
    return false;
  }
  auto _this{shared_from_this()};
  socket_.async_read_some(asio::buffer(receive_buffer_), [this, _this](const std::error_code& err_code,
                                                                       std::size_t data_length) {
    if (err_code && data_length <= 0) {
      LogError("read data from %s:%u error, reason:%s", this->socket_.remote_endpoint().address().to_string().c_str(),
               this->socket_.remote_endpoint().port(), err_code.message().c_str());
    } else if (err_code) {
      LogError("read data from %s:%u error, but remain %zu byte data, reason:%s",
               this->socket_.remote_endpoint().address().to_string().c_str(), this->socket_.remote_endpoint().port(),
               data_length, err_code.message().c_str());
    }
    LogDebug("receive %zu byte data", data_length);
    std::string data{this->receive_buffer_.begin(), this->receive_buffer_.begin() + data_length};
    auto read_result = this->packet_assemble_->data_received(TcpConnectionHandle::create(_this), std::move(data));
    if (read_result == DataReadResult::kDisconnect) {
      LogError("read data from %s:%u error, disconnect", this->socket_.remote_endpoint().address().to_string().c_str(),
               this->socket_.remote_endpoint().port());
      this->disconnect();
      return;
    } else if (read_result == DataReadResult::kError) {
      LogError("read data from %s:%u error, but continue read",
               this->socket_.remote_endpoint().address().to_string().c_str(), this->socket_.remote_endpoint().port());
    } else {
      LogDebug("read data from %s:%u success, continue read",
               this->socket_.remote_endpoint().address().to_string().c_str(), this->socket_.remote_endpoint().port());
    }
    this->read();
  });
  return true;
}

}  // namespace salt