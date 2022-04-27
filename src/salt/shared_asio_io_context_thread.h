#pragma once

#include <thread>

#include "asio.hpp"

namespace salt {
class SharedAsioIoContextThread {
 public:
  SharedAsioIoContextThread(asio::io_context& shared_io_context);
  ~SharedAsioIoContextThread();

 private:
  void _poll();

 private:
  asio::io_context& shared_io_context_;
  std::thread poll_thread_;
};
}  // namespace salt