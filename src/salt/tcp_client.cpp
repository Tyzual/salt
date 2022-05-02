#include "salt/tcp_client.h"

#include "salt/error.h"
#include "salt/log.h"

namespace salt {

#if 0
tcp_client_start_argument &
tcp_client_start_argument::add_server(std::string address_v4, uint16_t port) {
  servers_.insert(std::make_pair(std::move(address_v4), port));
  return *this;
}

tcp_client_start_argument &tcp_client_start_argument::add_server(
    std::initializer_list<std::pair<std::string, uint16_t>> servers) {
  for (auto &it : servers) {
    servers_.insert(std::move(it));
  }
  return *this;
}
#endif

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
      [this, address_v4 = std::move(address_v4), port, call_back]() {
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
      [this, connection = std::move(connection), call_back,
       address_v4 = std::move(address_v4),
       port](const std::error_code &err_code,
             asio::ip::tcp::resolver::results_type result) {
        asio::async_connect(
            connection->get_socket(), result,
            [this, call_back, address_v4 = std::move(address_v4), port,
             connection = std::move(connection)](
                const std::error_code &error_code,
                const asio::ip::tcp::endpoint &endpoint) {
              if (call_back)
                call_back(error_code);

              if (!error_code) {
                log_debug("connected to host:%s:%u",
                          endpoint.address().to_string().c_str(),
                          endpoint.port());
                connected_[{address_v4, port}] = std::move(connection);
              }
            });
      });
}

void tcp_client::disconnect(
    std::string address_v4, uint16_t port,
    std::function<void(const std::error_code &)> call_back) {
  control_thread_.get_io_context().post(
      [this, address_v4 = std::move(address_v4), port, call_back] {
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

} // namespace salt
