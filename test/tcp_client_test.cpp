#include "gtest/gtest.h"

#include "salt/core/tcp_client.h"

#if 0
TEST(tcp_client_start_argument _test, test_server_count) {
  salt::tcp_client_start_argument init_argument;
  init_argument.add_server("127.0.0.1", 2001)
      .add_server({{"127.0.0.1", 2001},
                   {"127.0.0.1", 2002},
                   std::make_pair("127.0.0.1", 2003)});
  ASSERT_EQ(init_argument.servers_.size(), 3);
}
#endif