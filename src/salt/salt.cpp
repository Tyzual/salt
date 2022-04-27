#include <iostream>

#include "salt/version.h"

void say_hello() {
  std::cout << "Hello, from salt!" << std::endl;
  std::cout << "version:" << salt::version << std::endl;
}
