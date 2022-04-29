#include "salt/error.h"

namespace salt {

class salt_error_category : public std::error_category {
public:
  const char *name() const noexcept override;
  std::string message(int ev) const override;
};

const char *salt_error_category ::name() const noexcept { return "salt"; }

std::string salt_error_category ::message(int ev) const {
  switch (static_cast<error_code>(ev)) {
  case error_code::success: {
    return "success";
  } break;
  case error_code::send_queue_full: {
    return "send queue is full";
  } break;
  case error_code::null_connection: {
    return "connection is nullptr";
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
