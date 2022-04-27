#include "salt/tcp_server.h"

#include <system_error>

#include "salt/log.h"
#include "salt/tcp_connection.h"

namespace salt {

tcp_server::tcp_server()
    : transfor_io_context_work_guard_(transfor_io_context_.get_executor()) {}

tcp_server::~tcp_server() { stop(); }

void tcp_server::stop() {
  accept_thread_.stop();
  transfor_io_context_.stop();
}

bool tcp_server::init(uint16_t listen_port, uint32_t io_thread_cnt /* = 1 */) {
  return init("", listen_port, io_thread_cnt);
}

bool tcp_server::init(const std::string &listen_ip_v4, uint16_t listen_port,
                      uint32_t io_thread_cnt /* = 1 */) {
  if (!listen_ip_v4.empty()) {
    std::error_code err_code;
    listen_ip_ = asio::ip::make_address_v4(listen_ip_v4, err_code);
    if (err_code) {
      log_error("parse ip address %s error, reason:%s", listen_ip_v4.c_str(),
                err_code.message().c_str());
      return false;
    }
  }
  listen_port_ = listen_port;
  log_debug("listen ip address:%s:%u", listen_ip_.to_string().c_str(),
            listen_port_);
  for (uint32_t i = 0; i < io_thread_cnt; ++i) {
    io_threads_.emplace_back(
        new shared_asio_io_context_thread(transfor_io_context_));
  }
  return true;
}

bool tcp_server::accept() {
  if (!acceptor_) {
    log_error("acceptor is nullptr");
    return false;
  }
  auto connection =
      tcp_connection::create(transfor_io_context_, assemble_creator_());
  if (!connection) {
    log_error("create connection error");
    return false;
  }
  acceptor_->async_accept(connection->get_socket(),
                          [this, connection](const std::error_code &err_code) {
                            if (!err_code) {
                              log_debug("accept success");
                              connection->read();
                            } else {
                              log_error("accept error, reason:%s",
                                        err_code.message().c_str());
                            }

                            this->accept();
                          });
  return true;
}

bool tcp_server::start() {
  if (!assemble_creator_) {
    log_error("assemble creator not set");
    return false;
  }
  if (!acceptor_) {
    acceptor_ = std::make_shared<asio::ip::tcp::acceptor>(
        accept_thread_.get_io_context(),
        asio::ip::tcp::endpoint(listen_ip_, listen_port_));
    return accept();
  } else {
    log_error("tcp_serveralready started, listen:%s:%u",
              listen_ip_.to_string().c_str(), listen_port_);
    return false;
  }
  return true;
}

void tcp_server::set_assemble_creator(
    std::function<base_packet_assemble *(void)> assemble_creator) {
  assemble_creator_ = assemble_creator;
}

} // namespace salt