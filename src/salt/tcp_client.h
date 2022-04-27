#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "salt/detail/tcp_client_detail.h"

namespace salt {

class TcpClient {
 public:
  TcpClient();

  void connect(const std::vector<EndPoint>& endpoints);

 private:
};
}  // namespace salt