#pragma once

#include "salt/connection_handle.h"

namespace salt {

enum class DataReadResult {
  kSuccess = 0,
  kError = 1,
  kDisconnect = 2,
};

class BasePacketAssemble {
 public:
  virtual DataReadResult data_received(std::shared_ptr<ConnectionHandle> connection, std::string s) = 0;
  virtual ~BasePacketAssemble() = default;
};

}  // namespace salt