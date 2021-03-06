#include "salt/core/error.h"

namespace salt {

class salt_error_category : public std::error_category {
public:
  const char *name() const noexcept override;
  std::string message(int ev) const override;
};

const char *salt_error_category::name() const noexcept { return "salt_error"; }

std::string salt_error_category::message(int ev) const {
  switch (static_cast<error_code>(ev)) {
  case error_code::success: {
    return "success";
  } break;
  case error_code::parse_ip_address_error: {
    return "parse ip address error";
  } break;
  case error_code::assemble_creator_not_set: {
    return "packet assemble creator not set";
  } break;
  case error_code::send_queue_full: {
    return "send queue is full";
  } break;
  case error_code::null_connection: {
    return "connection is nullptr";
  } break;
  case error_code::not_connected: {
    return "socket not connected";
  } break;
  case error_code::require_disconnecet: {
    return "packet assemble return disconnect";
  } break;
  case error_code::call_disconnect: {
    return "user call "
           "tcp_client::disconnect";
  } break;
  case error_code::body_size_error: {
    return "body size error";
  } break;
  case error_code::header_read_error: {
    return "read header error";
  } break;
  case error_code::internel_error: {
    return "this is a bug, please submit an issue in "
           "https://github.com/Tyzual/salt";
  } break;
  case error_code::assemble_create_reutrn_nullptr: {
    return "assemble creator return nullptr";
  } break;
  case error_code::already_started: {
    return "tcp server already started";
  } break;
  case error_code::acceptor_is_nullptr: {
    return "acceptor is nullptr";
  } break;
  default: {
    return "(unknown error)";
  } break;
  }
}

static const salt_error_category _network_error_category{};

std::error_code make_error_code(error_code ec) {
  return {static_cast<int>(ec), _network_error_category};
}

const std::error_category &error_category() { return _network_error_category; }

} // namespace salt
