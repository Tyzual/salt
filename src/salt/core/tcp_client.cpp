#include "salt/core/tcp_client.h"

#include <chrono>

#include "salt/core/error.h"
#include "salt/core/log.h"
#include "salt/util/call_back_wrapper.h"

namespace salt {

tcp_client::~tcp_client() { stop(); }

void tcp_client::stop() {
  control_thread_.stop();
  transfor_io_context_.stop();
}

tcp_client::tcp_client()
    : transfor_io_context_work_guard_(transfor_io_context_.get_executor()),
      resolver_(control_thread_.get_io_context()) {}

void tcp_client::init(uint32_t transfer_thread_count) {
  if (transfer_thread_count == 0) {
    transfer_thread_count = 1;
  }

  for (auto i = 0u; i < transfer_thread_count; ++i) {
    io_threads_.emplace_back(
        new shared_asio_io_context_thread(transfor_io_context_));
  }
}

void tcp_client::connect(std::string address_v4, uint16_t port,
                         const connection_meta &meta) {
  if (!assemble_creator_) {
    return;
  }

  connection_meta_impl meta_impl{meta, 0};
  control_thread_.get_io_context().post(
      [this, meta_impl, address_v4 = std::move(address_v4), port] {
        if (meta_impl.meta_.retry_when_connection_error &&
            (meta_impl.meta_.retry_forever ||
             (!meta_impl.meta_.retry_forever &&
              meta_impl.meta_.max_retry_cnt > 0)) &&
            meta_impl.meta_.retry_interval_s > 0) {
          connection_metas_[{address_v4, port}] = meta_impl;
        }
        _connect(std::move(address_v4), port);
      });
}

void tcp_client::connect(std::string address_v4, uint16_t port) {
  if (!assemble_creator_) {
    return;
  }
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port]() {
        _connect(std::move(address_v4), port);
      });
}

void tcp_client::_connect(std::string address_v4, uint16_t port) {

  auto connection = tcp_connection::create(
      transfor_io_context_, assemble_creator_(),
      [this](const std::string &remote_addr, uint16_t port,
             const std::error_code &error_code) {
        this->handle_connection_error(remote_addr, port, error_code);
      });

  if (!connection) {
    return;
  }

  connection->set_remote_address(address_v4);
  connection->set_remote_port(port);
  all_[{address_v4, port}] = connection;
  resolver_.async_resolve(
      address_v4, std::to_string(port),
      [this, connection = std::move(connection),
       address_v4 = std::move(address_v4),
       port](const std::error_code &err_code,
             asio::ip::tcp::resolver::results_type result) {
        if (err_code) {
          connection->handle_fail_connection(err_code);
          return;
        }

        asio::async_connect(
            connection->get_socket(), result,
            [this, address_v4 = std::move(address_v4), port,
             connection = std::move(connection)](
                const std::error_code &error_code,
                const asio::ip::tcp::endpoint &endpoint) {
              if (!error_code) {
                log_debug("connected to host:%s:%u",
                          endpoint.address().to_string().c_str(),
                          endpoint.port());
                connection->set_remote_address(endpoint.address().to_string());
                connection->set_remote_port(endpoint.port());
                {
                  std::error_code error_code;
                  const auto &local_endpoint =
                      connection->get_socket().local_endpoint(error_code);
                  if (!error_code) {
                    connection->set_local_address(
                        local_endpoint.address().to_string());
                    connection->set_local_port(local_endpoint.port());
                  }
                }

                control_thread_.get_io_context().post(
                    [this, address_v4, port, connection]() {
                      auto pos = connection_metas_.find({address_v4, port});
                      if (pos != connection_metas_.end()) {
                        pos->second.current_retry_cnt_ = 0;
                      }
                      connected_[{address_v4, port}] = connection;
                      notify_connected(address_v4, port);
                    });
                connection->read();
              } else {
                connection->handle_fail_connection(error_code);
              }
            });
      });
}

void tcp_client::disconnect(std::string address_v4, uint16_t port) {
  notify_disconnected(make_error_code(error_code::call_disconnect), address_v4,
                      port);
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port] {
        _disconnect(std::move(address_v4), port);
      });
}

void tcp_client::_disconnect(std::string address_v4, uint16_t port) {
  {
    auto pos = connected_.find({address_v4, port});
    if (pos != connected_.end()) {
      pos->second->disconnect();
      connected_.erase(pos);
    }
  }

  {
    auto pos = all_.find({address_v4, port});
    if (pos != all_.end()) {
      all_.erase(pos);
    }
  }
}

void tcp_client::set_assemble_creator(
    std::function<base_packet_assemble *(void)> assemble_creator) {
  assemble_creator_ = assemble_creator;
}

void tcp_client::broadcast(
    std::string data, std::function<void(const std::error_code &)> call_back) {
  control_thread_.get_io_context().post(
      [this, data = std::move(data), call_back = std::move(call_back)] {
        for (auto &connected : connected_) {
          connected.second->send(data, call_back);
        }
      });
}

void tcp_client::send(std::string address_v4, uint16_t port, std::string data,
                      std::function<void(const std::error_code &)> call_back) {
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port, data = std::move(data),
       call_back = std::move(call_back)]() {
        _send(std::move(address_v4), port, std::move(data), call_back);
      });
}

void tcp_client::_send(std::string address_v4, uint16_t port, std::string data,
                       std::function<void(const std::error_code &)> call_back) {
  auto pos = connected_.find({std::move(address_v4), port});
  if (pos == connected_.end()) {
    call(call_back, make_error_code(error_code::not_connected));
  } else {
    pos->second->send(std::move(data), std::move(call_back));
  }
}

void tcp_client::handle_connection_error(const std::string &remote_address,
                                         uint16_t remote_port,
                                         const std::error_code &error_code) {
  control_thread_.get_io_context().post(
      [this, address_v4 = remote_address, remote_port] {
        _disconnect(std::move(address_v4), remote_port);
      });

  notify_disconnected(error_code, remote_address, remote_port);
  log_error("connection remote address %s:%u error, reason:%s, disconnect",
            remote_address.c_str(), remote_port, error_code.message().c_str());

  auto reconnect = [this, remote_address, remote_port](uint32_t wait_second) {
    auto timer = std::make_shared<asio::steady_timer>(
        control_thread_.get_io_context(), std::chrono::seconds(wait_second));
    timer->async_wait([this, timer, remote_address = std::move(remote_address),
                       remote_port](const std::error_code &) {
      connect(std::move(remote_address), remote_port);
    });
  };

  control_thread_.get_io_context().post([this, remote_address, remote_port,
                                         reconnect] {
    auto pos = connection_metas_.find({remote_address, remote_port});
    if (pos == connection_metas_.end()) {
      log_debug("drop connection %s:%u", remote_address.c_str(), remote_port);
      notify_dropped(remote_address, remote_port);
      return;
    } else {
      if (!pos->second.meta_.retry_when_connection_error) {
        log_debug("drop connection %s:%u", remote_address.c_str(), remote_port);
        notify_dropped(remote_address, remote_port);
        return;
      } else {
        if (!pos->second.meta_.retry_forever) {
          if (pos->second.current_retry_cnt_ <
              pos->second.meta_.max_retry_cnt) {
            ++(pos->second.current_retry_cnt_);
            log_debug(
                "try reconnection to %s:%u for %u times after %u seconds, "
                "max retry count:%u",
                remote_address.c_str(), remote_port,
                pos->second.current_retry_cnt_,
                pos->second.meta_.retry_interval_s,
                pos->second.meta_.max_retry_cnt);
            reconnect(pos->second.meta_.retry_interval_s);
          } else {
            log_error("connection remote address %s:%u reaches max retry "
                      "count:%u, drop connection",
                      remote_address.c_str(), remote_port,
                      pos->second.meta_.max_retry_cnt);
            notify_dropped(remote_address, remote_port);
            return;
          }
        } else {
          log_debug("reconnection to %s:%u after %u seconds",
                    remote_address.c_str(), remote_port,
                    pos->second.meta_.retry_interval_s);
          reconnect(pos->second.meta_.retry_interval_s);
        }
      }
    }
  });
}

void tcp_client::set_notify(tcp_client_notify *notify) { notify_ = notify; }

void tcp_client::notify_connected(const std::string &remote_addr,
                                  uint16_t remote_port) {
  if (notify_)
    notify_->connection_connected(remote_addr, remote_port);
}

void tcp_client::notify_disconnected(const std::error_code &error_code,
                                     const std::string &remote_addr,
                                     uint16_t remote_port) {
  if (notify_)
    notify_->connection_disconnected(error_code, remote_addr, remote_port);
}

void tcp_client::notify_dropped(const std::string &remote_addr,
                                uint16_t remote_port) {
  if (notify_)
    notify_->connection_dropped(remote_addr, remote_port);
}

} // namespace salt
