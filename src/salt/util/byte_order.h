#pragma once

#include <arpa/inet.h>

namespace salt {

/**
 * @brief 字节序相关工具
 *
 */
namespace byte_order {

/**
 * @brief 将数据转换为网络字节序
 *
 * @param val 需要转换的数据
 * @return uint32_t 转换后的数据
 */
inline uint32_t to_network(uint32_t val) { return htonl(val); }

/**
 * @brief 将数据转换为网络字节序
 *
 * @param val 需要转换的数据
 * @return uint16_t 转换后的数据
 */
inline uint16_t to_network(uint16_t val) { return htons(val); }

/**
 * @brief 将数据转换为网络字节序
 *
 * @param val 需要转换的数据
 * @return uint8_t 转换后的数据
 */
inline uint8_t to_network(uint8_t val) { return val; };

/**
 * @brief 将数据转换为主机字节序
 *
 * @param val 需要转换的数据
 * @return uint32_t 转换后的数据
 */
inline uint32_t to_host(uint32_t val) { return ntohl(val); }

/**
 * @brief 将数据转换为主机字节序
 *
 * @param val 需要转换的数据
 * @return uint16_t 转换后的数据
 */
inline uint16_t to_host(uint16_t val) { return ntohs(val); }

/**
 * @brief 将数据转换为主机字节序
 *
 * @param val 需要转换的数据
 * @return uint8_t 转换后的数据
 */
inline uint8_t to_host(uint8_t val) { return val; };

} // namespace byte_order

} // namespace salt
