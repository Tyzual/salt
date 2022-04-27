#include "salt/shared_asio_io_context_thread.h"

#include "salt/log.h"

namespace salt {

SharedAsioIoContextThread::SharedAsioIoContextThread(asio::io_context& shared_io_context)
    : shared_io_context_(shared_io_context), poll_thread_{[this]() { this->_poll(); }} {}

void SharedAsioIoContextThread::_poll() {
  while (true) {
    try {
      this->shared_io_context_.run();
      return;
    } catch (std::exception& e) {
      LogError("io context run throw exception:%s", e.what());
    }
  }
}

SharedAsioIoContextThread::~SharedAsioIoContextThread() {
  if (poll_thread_.joinable()) poll_thread_.join();
}

}  // namespace salt