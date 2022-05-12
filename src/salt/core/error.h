#pragma once

#include <system_error>

namespace salt {

enum class error_code {
  success = 0,
  parse_ip_address_error,
  assemble_creator_not_set,
  send_queue_full,
  null_connection,
  not_connected,
  require_disconnecet,
  call_disconnect,
};

std::error_code make_error_code(error_code ec);
const std::error_category &error_category();

} // namespace salt
