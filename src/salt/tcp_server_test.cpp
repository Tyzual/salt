#include <gtest/gtest.h>
#include <salt/tcp_server.h>

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(TcpServerTest, test_init_1) {
  ::salt::TcpServer server;
  auto success = server.init(123, 1);
  ASSERT_TRUE(success);
  ASSERT_EQ(server.get_listen_address(), "0.0.0.0");
  ASSERT_EQ(server.get_listen_port(), 123);
  server.stop();
}

TEST(TcpServerTest, test_init_2) {
  ::salt::TcpServer server;
  auto success = server.init("127.0.0.1", 123, 1);
  ASSERT_TRUE(success);
  ASSERT_EQ(server.get_listen_address(), "127.0.0.1");
  ASSERT_EQ(server.get_listen_port(), 123);
  server.stop();
}

TEST(TcpServerTest, test_init_3) {
  ::salt::TcpServer server;
  auto success = server.init("256.0.0.1", 123, 1);
  ASSERT_FALSE(success);
}

TEST(TcpServerTest, test_init_4) {
  ::salt::TcpServer server;
  auto success = server.init("256.01", 123, 1);
  ASSERT_FALSE(success);
  server.stop();
}

TEST(TcpServerTest, test_init_5) {
  ::salt::TcpServer server;
  auto success = server.init("127.0.0.0.1", 123, 1);
  ASSERT_FALSE(success);
  server.stop();
}