#pragma once

#include <stdio.h>

#ifdef ENABLE_LOG
#define log_error(fmt, ...)                                                    \
  fprintf(stderr, "%s:%u [ERROR] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);

#define log_debug(fmt, ...)                                                    \
  printf("%s:%u [DEBUG] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif