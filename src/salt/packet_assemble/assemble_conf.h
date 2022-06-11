#pragma once

namespace salt {

/**
 * @brief body长度的计算模式
 */
enum class body_length_calc_mode {

  /**
   * @brief 长度只包含body本身
   *        使用这种模式会忽略
   *        header_body_assemble::reserve_body_size_
   *        字段
   */
  body_only = 1,

  /**
   * @brief 长度包含body + sizeof(长度字段)
   *        这种方式的包内容长度计算公式为 接收到的包内容长度 -
   * sizeof(包长度字段) 使用这种模式会忽略
   *        header_body_assemble::reserve_body_size_
   *        字段
   */
  with_length_field,

  /**
   * @brief 长度包含body + sizeof(包头)
   *        这种方式的包内容长度计算公式为 接收到的包内容长度 - sizeof(包头)
   *        使用这种模式会忽略
   *        header_body_assemble::reserve_body_size_
   *        字段
   */
  with_header,

  /**
   * @brief 长度包含body + 自定义长度
   *        这种方式的包内容长度计算公式为 接收到的包内容长度 -
   * header_body_assemble::reserve_body_size_ 自定义长度由
   * header_body_assemble::reserve_body_size_ 指定
   */
  custom_length,
};

} // namespace salt