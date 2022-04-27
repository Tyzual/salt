
#pragma once

#include <functional>
#include <string>
#include <system_error>

namespace salt {

class ConnectionHandle {
 public:
  virtual void send(std::string data, std::function<void(const std::error_code&)> call_back) = 0;
  virtual ~ConnectionHandle() = default;
};

}  // namespace salt