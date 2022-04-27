#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "salt/tcp_connection.h"

namespace salt {

class EndPoint {
 public:
  std::string address_v4;
  uint16_t port;
};

class ConnectionInfo {
 private:
  std::map<EndPoint, std::shared_ptr<TcpConnection>> connected_;
  std::map<EndPoint, std::shared_ptr<TcpConnection>> connecting_;
};
}  // namespace salt