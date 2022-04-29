#include "salt/tcp_client.h"

int main() {
  salt::tcp_client_init_argument init_argument;
  init_argument.add_server("127.0.0.1", 2001)
      .add_server({{"127.0.0.1", 2001},
                   {"127.0.0.1", 2002},
                   std::make_pair("127.0.0.1", 2003)});
  return 0;
}