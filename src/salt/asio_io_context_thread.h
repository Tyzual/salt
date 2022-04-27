#pragma once

#include "asio.hpp"

namespace salt {
class AsioIoContextThread {
 public:
  AsioIoContextThread();
  ~AsioIoContextThread();

  void stop();

  inline asio::io_context& get_io_context() { return io_context_; }

 private:
  asio::io_context io_context_;
  std::thread poll_thread_;
};
}  // namespace salt