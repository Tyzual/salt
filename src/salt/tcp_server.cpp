#include "salt/tcp_server.h"

#include <system_error>

#include "salt/log.h"
#include "salt/tcp_connection.h"

namespace salt {

TcpServer::TcpServer() : transfor_io_context_work_guard_(transfor_io_context_.get_executor()) {}

TcpServer::~TcpServer() { stop(); }

void TcpServer::stop() {
  accept_thread_.stop();
  transfor_io_context_.stop();
}

bool TcpServer::init(uint16_t listen_port, uint32_t io_thread_cnt /* = 1 */) {
  return init("", listen_port, io_thread_cnt);
}

bool TcpServer::init(const std::string& listen_ip_v4, uint16_t listen_port, uint32_t io_thread_cnt /* = 1 */) {
  if (!listen_ip_v4.empty()) {
    std::error_code err_code;
    listen_ip_ = asio::ip::make_address_v4(listen_ip_v4, err_code);
    if (err_code) {
      LogError("parse ip address %s error, reason:%s", listen_ip_v4.c_str(), err_code.message().c_str());
      return false;
    }
  }
  listen_port_ = listen_port;
  LogDebug("listen ip address:%s:%u", listen_ip_.to_string().c_str(), listen_port_);
  for (uint32_t i = 0; i < io_thread_cnt; ++i) {
    io_threads_.emplace_back(new SharedAsioIoContextThread(transfor_io_context_));
  }
  return true;
}

bool TcpServer::accept() {
  if (!acceptor_) {
    LogError("acceptor is nullptr");
    return false;
  }
  auto connection = TcpConnection::create(transfor_io_context_, assemble_creator_());
  if (!connection) {
    LogError("create connection error");
    return false;
  }
  acceptor_->async_accept(connection->get_socket(), [this, connection](const std::error_code& err_code) {
    if (!err_code) {
      LogDebug("accept success");
      connection->read();
    } else {
      LogError("accept error, reason:%s", err_code.message().c_str());
    }

    this->accept();
  });
  return true;
}

bool TcpServer::start() {
  if (!assemble_creator_) {
    LogError("assemble creator not set");
    return false;
  }
  if (!acceptor_) {
    acceptor_ = std::make_shared<asio::ip::tcp::acceptor>(accept_thread_.get_io_context(),
                                                          asio::ip::tcp::endpoint(listen_ip_, listen_port_));
    return accept();
  } else {
    LogError("TcpServer already started, listen:%s:%u", listen_ip_.to_string().c_str(), listen_port_);
    return false;
  }
  return true;
}

void TcpServer::set_assemble_creator(std::function<BasePacketAssemble*(void)> assemble_creator) {
  assemble_creator_ = assemble_creator;
}

}  // namespace salt