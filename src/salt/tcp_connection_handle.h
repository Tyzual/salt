#pragma once

#include "salt/connection_handle.h"
#include "salt/tcp_connection.h"

namespace salt {

class tcp_connection_handle : public connection_handle {
public:
  static std::shared_ptr<connection_handle>
  create(std::shared_ptr<tcp_connection> connection);
  void send(std::string data,
            std::function<void(const std::error_code &)> call_back) override;
  ~tcp_connection_handle() override = default;

private:
  tcp_connection_handle(std::shared_ptr<tcp_connection> connection);

private:
  std::shared_ptr<tcp_connection> connection_{nullptr};
};

} // namespace salt