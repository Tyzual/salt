#include "salt/tcp_connection_handle.h"
#include "salt/error.h"
#include "salt/util/call_back_wrapper.h"

namespace salt {

void tcp_connection_handle::send(
    uint32_t seq, std::string data,
    std::function<void(uint32_t seq, const std::error_code &)> call_back) {
  if (!connection_) {
    if (call_back) {
      call(call_back, seq, make_error_code(error_code::null_connection));
    }
    return;
  }

  connection_->send(seq, std::move(data), call_back);
}

tcp_connection_handle::tcp_connection_handle(
    std::shared_ptr<tcp_connection> connection)
    : connection_(std::move(connection)) {}

std::shared_ptr<connection_handle>
tcp_connection_handle::create(std::shared_ptr<tcp_connection> connection) {
  return std::shared_ptr<connection_handle>{
      new tcp_connection_handle(std::move(connection))};
}

} // namespace salt