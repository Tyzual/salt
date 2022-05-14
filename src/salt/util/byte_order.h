#pragma once

#include <arpa/inet.h>

namespace salt {

namespace byte_order {

inline uint32_t to_network(uint32_t val) { return htonl(val); }
inline uint16_t to_network(uint16_t val) { return htons(val); }
inline uint8_t to_network(uint8_t val) { return val; };
inline uint32_t to_host(uint32_t val) { return ntohl(val); }
inline uint16_t to_host(uint16_t val) { return ntohs(val); }
inline uint8_t to_host(uint8_t val) { return val; };

} // namespace byte_order

} // namespace salt
