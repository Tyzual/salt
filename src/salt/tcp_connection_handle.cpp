#include "salt/tcp_connection_handle.h"
#include "salt/error.h"

namespace salt {

void TcpConnectionHandle::send(std::string data, std::function<void(const std::error_code&)> call_back) {
  if (!connection_) {
    if (call_back) {
      call_back(make_error_code(ErrorCode::kNullConnection));
    }
    return;
  }

  connection_->send(std::move(data), call_back);
}

TcpConnectionHandle::TcpConnectionHandle(std::shared_ptr<TcpConnection> connection)
    : connection_(std::move(connection)) {}

std::shared_ptr<ConnectionHandle> TcpConnectionHandle::create(std::shared_ptr<TcpConnection> connection) {
  return std::shared_ptr<ConnectionHandle>{new TcpConnectionHandle(std::move(connection))};
}

}  // namespace salt