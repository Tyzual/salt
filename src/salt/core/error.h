#pragma once

#include <system_error>

namespace salt {

/**
 * @brief salt错误码枚举
 *
 */
enum class error_code {
  /**
   * @brief 成功
   *
   */
  success = 0,

  /**
   * @brief 解析 ip 错误
   *
   */
  parse_ip_address_error,

  /**
   * @brief packet assemble creator 为空
   *
   */
  assemble_creator_not_set,

  /**
   * @brief 发送队列满
   *
   */
  send_queue_full,

  /**
   * @brief 空链接
   *
   */
  null_connection,

  /**
   * @brief 未连接
   *
   */
  not_connected,

  /**
   * @brief 拆包器返回断开链接
   *
   */
  require_disconnecet,

  /**
   * @brief 用户调用 tcp_client::disconnect
   *
   */
  call_disconnect,

  /**
   * @brief 包内容长度异常
   *
   */
  body_size_error,

  /**
   * @brief 解析包头异常
   *
   */
  header_read_error,

  /**
   * @brief 内部错误，请在 https://github.com/Tyzual/salt 上提 issue
   *
   */
  internel_error,

  /**
   * @brief 拆包器工厂返回空指针
   *
   */
  assemble_create_reutrn_nullptr,

  /**
   * @brief tcp_server 已经启动
   *
   */
  already_started,

  /**
   * @brief acceptor 为空指针
   *
   */
  acceptor_is_nullptr,
};

/**
 * @brief 创建std::error_code
 *
 * @param ec error_code 枚举
 * @return std::error_code 创建好的error_code
 */
std::error_code make_error_code(error_code ec);

/**
 * @brief salt的error_category
 *
 * @return const std::error_category& salt的error_category
 */
const std::error_category &error_category();

} // namespace salt
