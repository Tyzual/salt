#include "salt/error.h"

namespace salt {

class NetWorkErrorCategory : public std::error_category {
 public:
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* NetWorkErrorCategory::name() const noexcept { return "tre_salt"; }

std::string NetWorkErrorCategory::message(int ev) const {
  switch (static_cast<ErrorCode>(ev)) {
    case ErrorCode::kSuccess: {
      return "success";
    } break;
    case ErrorCode::kSendQueueFull: {
      return "send queue is full";
    } break;
    case ErrorCode::kNullConnection: {
      return "connection is nullptr";
    } break;
    default: {
      return "(unknown error)";
    } break;
  }
}

static const NetWorkErrorCategory _network_error_category{};

std::error_code make_error_code(ErrorCode ec) { return {static_cast<int>(ec), _network_error_category}; }

const std::error_category& error_category() { return _network_error_category; }

}  // namespace salt