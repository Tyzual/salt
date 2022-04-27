#pragma once

#include <thread>

#include "asio.hpp"

namespace salt {
class shared_asio_io_context_thread {
public:
  shared_asio_io_context_thread(asio::io_context &shared_io_context);
  ~shared_asio_io_context_thread();

private:
  void _poll();

private:
  asio::io_context &shared_io_context_;
  std::thread poll_thread_;
};
} // namespace salt