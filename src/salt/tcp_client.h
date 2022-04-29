#pragma once

#include <initializer_list>
#include <set>
#include <string>

#include "asio.hpp"

#include "salt/asio_io_context_thread.h"
#include "salt/shared_asio_io_context_thread.h"

namespace salt {

class tcp_client_init_argument {
public:
  tcp_client_init_argument &add_server(std::string address_v4, uint16_t port);

  tcp_client_init_argument &
  add_server(std::initializer_list<std::pair<std::string, uint16_t>> servers);

  inline tcp_client_init_argument &
  set_io_thread_count(uint32_t io_thread_count) {
    io_thread_count_ = io_thread_count;
    return *this;
  }

private:
  std::set<std::pair<std::string, uint16_t>> servers_;
  uint32_t io_thread_count_;
};

class tcp_client {
public:
  tcp_client();

private:
  asio::io_context transfor_io_context_;
  asio::executor_work_guard<asio::io_context::executor_type>
      transfor_io_context_work_guard_;
  asio_io_context_thread control_thread_;
  std::vector<std::shared_ptr<shared_asio_io_context_thread>> io_threads_;
};

} // namespace salt