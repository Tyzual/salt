#pragma once

#include "salt/connection_handle.h"
#include "salt/tcp_connection.h"

namespace salt {

class TcpConnectionHandle : public ConnectionHandle {
 public:
  static std::shared_ptr<ConnectionHandle> create(std::shared_ptr<TcpConnection> connection);
  void send(std::string data, std::function<void(const std::error_code&)> call_back) override;
  ~TcpConnectionHandle() override = default;

 private:
  TcpConnectionHandle(std::shared_ptr<TcpConnection> connection);

 private:
  std::shared_ptr<TcpConnection> connection_{nullptr};
};

}  // namespace salt