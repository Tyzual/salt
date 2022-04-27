#pragma once

#include <system_error>

namespace salt {

enum class error_code {
  success = 0,
  send_queue_full,
  null_connection,
};

std::error_code make_error_code(error_code ec);
const std::error_category &error_category();

} // namespace salt
