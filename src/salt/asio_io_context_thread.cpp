#include "salt/asio_io_context_thread.h"

namespace salt {

AsioIoContextThread::AsioIoContextThread()
    : poll_thread_{[this]() {
        asio::executor_work_guard<asio::io_context::executor_type> work_guard(this->io_context_.get_executor());
        this->io_context_.run();
      }} {}

AsioIoContextThread::~AsioIoContextThread() { stop(); }

void AsioIoContextThread::stop() {
  io_context_.stop();
  if (poll_thread_.joinable()) poll_thread_.join();
}

}  // namespace salt