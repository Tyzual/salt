#include "salt/tcp_client.h"

#include "salt/error.h"
#include "salt/log.h"
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

void tcp_client::connect(
    std::string address_v4, uint16_t port, const connection_meta &meta,
    std::function<void(const std::error_code &)> call_back) {
  if (!assemble_creator_) {
    call(call_back, make_error_code(error_code::assemble_creator_not_set));
    return;
  }

  connection_meta_impl meta_impl{meta, 0};
  control_thread_.get_io_context().post(
      [this, meta_impl, address_v4 = std::move(address_v4), port,
       call_back = std::move(call_back)] {
        if (meta_impl.meta_.retry_when_connection_error &&
            (meta_impl.meta_.retry_forever ||
             (!meta_impl.meta_.retry_forever &&
              meta_impl.meta_.max_retry_cnt > 0)) &&
            meta_impl.meta_.retry_interval_s > 0) {
          connection_metas_[{address_v4, port}] = meta_impl;
        }
        _connect(std::move(address_v4), port, std::move(call_back));
      });
}

void tcp_client::connect(
    std::string address_v4, uint16_t port,
    std::function<void(const std::error_code &)> call_back) {
  if (!assemble_creator_) {
    call(call_back, make_error_code(error_code::assemble_creator_not_set));
    return;
  }
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port,
       call_back = std::move(call_back)]() {
        _connect(std::move(address_v4), port, call_back);
      });
}

void tcp_client::_connect(
    std::string address_v4, uint16_t port,
    std::function<void(const std::error_code &)> call_back) {
  auto connection = tcp_connection::create(
      transfor_io_context_, assemble_creator_(),
      [this](const std::string &remote_addr, uint16_t port,
             const std::error_code &error_code) {
        this->handle_connection_error(remote_addr, port, error_code);
      });
  connection->set_remote_address(address_v4);
  connection->set_remote_port(port);
  all_[{address_v4, port}] = connection;
  resolver_.async_resolve(
      address_v4, std::to_string(port),
      [this, connection = std::move(connection),
       call_back = std::move(call_back), address_v4 = std::move(address_v4),
       port](const std::error_code &err_code,
             asio::ip::tcp::resolver::results_type result) {
        if (err_code) {
          call(call_back, err_code);
          connection->handle_fail_connection(err_code);
          return;
        }

        asio::async_connect(
            connection->get_socket(), result,
            [this, call_back = std::move(call_back),
             address_v4 = std::move(address_v4), port,
             connection = std::move(connection)](
                const std::error_code &error_code,
                const asio::ip::tcp::endpoint &endpoint) {
              call(call_back, error_code);

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
                connected_[{address_v4, port}] = connection;
                connection->read();
              } else {
                connection->handle_fail_connection(error_code);
              }
            });
      });
}

void tcp_client::disconnect(
    std::string address_v4, uint16_t port,
    std::function<void(const std::error_code &)> call_back) {
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port,
       call_back = std::move(call_back)] {
        _disconnect(std::move(address_v4), port, call_back);
      });
}

void tcp_client::_disconnect(
    std::string address_v4, uint16_t port,
    std::function<void(const std::error_code &)> call_back) {
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
      call(call_back, make_error_code(error_code::success));
    } else {
      call(call_back, make_error_code(error_code::not_connected));
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
  log_error("connection remote address %s:%u error, reason:%s, disconnect",
            remote_address.c_str(), remote_port, error_code.message().c_str());
  disconnect(remote_address, remote_port,
             [remote_address, remote_port](const std::error_code &err_code) {
               if (err_code) {
                 log_error("disconnect from %s:%u error, reason:%s",
                           remote_address.c_str(), remote_port,
                           err_code.message().c_str());
               } else {
                 log_debug("disconnect from %s:%u success",
                           remote_address.c_str(), remote_port);
               }
             });
}

} // namespace salt
