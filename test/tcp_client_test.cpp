#include <gtest/gtest.h>

#include "salt/tcp_client.h"

TEST(tcp_client_init_argument_test, test_server_count) {
  salt::tcp_client_init_argument init_argument;
  init_argument.add_server("127.0.0.1", 2001)
      .add_server({{"127.0.0.1", 2001},
                   {"127.0.0.1", 2002},
                   std::make_pair("127.0.0.1", 2003)});
  ASSERT_EQ(init_argument.servers_.size(), 3);
}