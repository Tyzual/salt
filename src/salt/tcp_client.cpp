#include "salt/tcp_client.h"

namespace salt {

tcp_client_init_argument &
tcp_client_init_argument::add_server(std::string address_v4, uint16_t port) {
  servers_.insert(std::make_pair(std::move(address_v4), port));
  return *this;
}

tcp_client_init_argument &tcp_client_init_argument::add_server(
    std::initializer_list<std::pair<std::string, uint16_t>> servers) {
  for (auto &it : servers) {
    servers_.insert(std::move(it));
  }
  return *this;
}

tcp_client::tcp_client()
    : transfor_io_context_work_guard_(transfor_io_context_.get_executor()) {}

} // namespace salt
