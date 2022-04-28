#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <system_error>

namespace salt {

class connection_handle {
public:
  virtual void send(
      uint32_t seq, std::string data,
      std::function<void(uint32_t seq, const std::error_code &)> call_back) = 0;
  virtual ~connection_handle() = default;
};

} // namespace salt