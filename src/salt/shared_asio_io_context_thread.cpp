#include "salt/shared_asio_io_context_thread.h"

#include "salt/log.h"

namespace salt {

shared_asio_io_context_thread::shared_asio_io_context_thread(
    asio::io_context &shared_io_context)
    : shared_io_context_(shared_io_context), poll_thread_{
                                                 [this]() { this->_poll(); }} {}

void shared_asio_io_context_thread::_poll() {
  while (true) {
    try {
      this->shared_io_context_.run();
      return;
    } catch (std::exception &e) {
      log_error("io context run throw exception:%s", e.what());
    }
  }
}

shared_asio_io_context_thread::~shared_asio_io_context_thread() {
  if (poll_thread_.joinable())
    poll_thread_.join();
}

} // namespace salt