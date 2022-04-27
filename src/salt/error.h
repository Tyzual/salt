#pragma once

#include <system_error>

namespace salt {

enum class ErrorCode {
  kSuccess = 0,
  kSendQueueFull,
  kNullConnection,
};

std::error_code make_error_code(ErrorCode ec);
const std::error_category& error_category();

}  // namespace salt
