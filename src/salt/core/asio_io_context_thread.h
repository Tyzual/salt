#pragma once

#include "asio.hpp"

namespace salt {
class asio_io_context_thread {
public:
  asio_io_context_thread();
  ~asio_io_context_thread();

  void stop();

  inline asio::io_context &get_io_context() { return io_context_; }

private:
  void run();

private:
  asio::io_context io_context_;
  std::thread poll_thread_;
};
} // namespace salt