#include "salt/asio_io_context_thread.h"

namespace salt {

asio_io_context_thread::asio_io_context_thread() { run(); }

asio_io_context_thread::~asio_io_context_thread() { stop(); }

void asio_io_context_thread::run() {
  poll_thread_ = std::thread{[this]() {
    asio::executor_work_guard<asio::io_context::executor_type> work_guard(
        io_context_.get_executor());
    io_context_.run();
  }};
}

void asio_io_context_thread::stop() {
  io_context_.stop();
  if (poll_thread_.joinable()) {
    poll_thread_.join();
  }
}

} // namespace salt