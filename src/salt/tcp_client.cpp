#include "salt/tcp_client.h"

#include "salt/error.h"
#include "salt/log.h"

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
    std::string address_v4, uint16_t port,
    std::function<void(const std::error_code &)> call_back) {
  if (!assemble_creator_) {
    call_back(make_error_code(error_code::assemble_creator_not_set));
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
  auto connection =
      tcp_connection::create(transfor_io_context_, assemble_creator_());
  all_[{address_v4, port}] = connection;
  resolver_.async_resolve(
      address_v4, std::to_string(port),
      [this, connection = std::move(connection),
       call_back = std::move(call_back), address_v4 = std::move(address_v4),
       port](const std::error_code &err_code,
             asio::ip::tcp::resolver::results_type result) {
        asio::async_connect(
            connection->get_socket(), result,
            [this, call_back = std::move(call_back),
             address_v4 = std::move(address_v4), port,
             connection = std::move(connection)](
                const std::error_code &error_code,
                const asio::ip::tcp::endpoint &endpoint) {
              if (call_back)
                call_back(error_code);

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
                addr_v4 key{address_v4, port};
                connected_[key] = connection;
                connection->read();
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
      call_back(make_error_code(error_code::success));
    } else {
      call_back(make_error_code(error_code::not_connected));
    }
  }
}

void tcp_client::set_assemble_creator(
    std::function<base_packet_assemble *(void)> assemble_creator) {
  assemble_creator_ = assemble_creator;
}

void tcp_client::broadcast(
    uint32_t seq, std::string data,
    std::function<void(uint32_t seq, const std::error_code &)> call_back) {
  control_thread_.get_io_context().post(
      [this, seq, data = std::move(data), call_back = std::move(call_back)] {
        for (auto &connected : connected_) {
          connected.second->send(seq, data, call_back);
        }
      });
}

void tcp_client::send(
    std::string address_v4, uint16_t port, uint32_t seq, std::string data,
    std::function<void(uint32_t seq, const std::error_code &)> call_back) {
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port, seq,
       data = std::move(data), call_back = std::move(call_back)]() {
        _send(std::move(address_v4), port, seq, std::move(data), call_back);
      });
}

void tcp_client::_send(
    std::string address_v4, uint16_t port, uint32_t seq, std::string data,
    std::function<void(uint32_t seq, const std::error_code &)> call_back) {
  auto pos = connected_.find({std::move(address_v4), port});
  if (pos == connected_.end()) {
    call_back(seq, make_error_code(error_code::not_connected));
  } else {
    pos->second->send(seq, std::move(data), std::move(call_back));
  }
}

} // namespace salt
