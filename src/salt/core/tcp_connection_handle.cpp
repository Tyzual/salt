#include "salt/core/tcp_connection_handle.h"
#include "salt/core/error.h"
#include "salt/util/call_back_wrapper.h"

namespace salt {

void tcp_connection_handle::send(
    std::string data, std::function<void(const std::error_code &)> call_back) {
  if (!connection_) {
    call(call_back, make_error_code(error_code::null_connection));
    return;
  }

  connection_->send(std::move(data), std::move(call_back));
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