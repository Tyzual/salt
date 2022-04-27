#pragma once

#include <stdio.h>

#define LogError(fmt, args, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__);

#define LogDebug(fmt, args, ...) printf(fmt "\n", ##__VA_ARGS__);