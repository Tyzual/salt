#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <system_error>

namespace salt {

class connection_handle {
public:
  virtual void send(std::string data,
                    std::function<void(const std::error_code &)> call_back) = 0;
  virtual ~connection_handle() = default;
};

} // namespace salt