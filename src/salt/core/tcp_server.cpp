#include "salt/core/tcp_server.h"

#include <system_error>

#include "salt/core/error.h"
#include "salt/core/log.h"
#include "salt/core/tcp_connection.h"

namespace salt {

tcp_server::tcp_server()
    : transfer_io_context_work_guard_(transfer_io_context_.get_executor()) {}

tcp_server::~tcp_server() { stop(); }

void tcp_server::stop() {
  accept_thread_.stop();
  transfer_io_context_.stop();
}

bool tcp_server::init(uint16_t listen_port, uint32_t io_thread_cnt /* = 1 */) {
  set_listen_port(listen_port).set_transfer_thread_count(io_thread_cnt);
  return true;
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
        new shared_asio_io_context_thread(transfer_io_context_));
  }
  return true;
}

tcp_server &tcp_server::set_listen_ip_v4(const std::string &listen_ip_v4) {
  listen_ip_ = asio::ip::make_address_v4(listen_ip_v4);
  return *this;
}

tcp_server &tcp_server::set_listen_port(uint16_t listen_port) {
  listen_port_ = listen_port;
  return *this;
}

tcp_server &
tcp_server::set_transfer_thread_count(uint32_t transfer_thread_count) {
  if (transfer_thread_count == 0) {
    log_info("transfer_thread_count is 0, change to 1");
    transfer_thread_count = 1;
  }

  if (io_threads_.size() != 0) {
    log_info("you can set transfer thread count only once, ignore this core");
    return *this;
  }

  for (auto i = 0u; i < transfer_thread_count; ++i) {
    io_threads_.emplace_back(
        new shared_asio_io_context_thread(transfer_io_context_));
  }

  return *this;
}

std::error_code tcp_server::accept() {
  if (!acceptor_) {
    log_error("acceptor is nullptr");
    return make_error_code(error_code::acceptor_is_nullptr);
  }
  auto connection =
      tcp_connection::create(transfer_io_context_, assemble_creator_());
  if (!connection) {
    log_error("create connection error");
    return make_error_code(error_code::internel_error);
  }
  acceptor_->async_accept(
      connection->get_socket(),
      [this, connection](const std::error_code &err_code) {
        if (!err_code) {
          {
            std::error_code error_code;
            const auto &local_endpoint =
                connection->get_socket().local_endpoint(error_code);
            if (!error_code) {
              connection->set_local_address(
                  local_endpoint.address().to_string());
              connection->set_local_port(local_endpoint.port());
            }
            const auto &remote_endpoint =
                connection->get_socket().remote_endpoint(error_code);
            if (!error_code) {
              connection->set_remote_address(
                  remote_endpoint.address().to_string());
              connection->set_remote_port(remote_endpoint.port());
            }
          }
          log_debug("accept new connection from %s:%u",
                    connection->get_remote_address().c_str(),
                    connection->get_remote_port());
          connection->read();
        } else {
          log_error("accept error, reason:%s", err_code.message().c_str());
        }

        this->accept();
      });
  return make_error_code(error_code::success);
}

std::error_code tcp_server::start() {
  if (!assemble_creator_) {
    log_error("assemble creator not set");
    return make_error_code(error_code::assemble_creator_not_set);
  }

  if (io_threads_.empty()) {
    set_transfer_thread_count(1);
  }

  if (!acceptor_) {
    acceptor_ = std::make_shared<asio::ip::tcp::acceptor>(
        accept_thread_.get_io_context(),
        asio::ip::tcp::endpoint(listen_ip_, listen_port_));
    return accept();
  } else {
    log_error("tcp_server already started, listen:%s:%u",
              listen_ip_.to_string().c_str(), listen_port_);
    return make_error_code(error_code::already_started);
  }
  return make_error_code(error_code::success);
}

tcp_server &tcp_server::set_assemble_creator(
    std::function<base_packet_assemble *(void)> assemble_creator) {
  assemble_creator_ = assemble_creator;
  return *this;
}

} // namespace salt