#pragma once

#include <memory>
#include <string>

#include "salt/core/connection_handle.h"

namespace salt {

/**
 * @brief 拆包器处理数据结果
 *
 */
enum class data_read_result {
  /**
   * @brief 处理数据成功，继续读取数据
   *
   */
  success = 0,

  /**
   * @brief 处理数据异常，但是可以继续读取数据
   *
   */
  error = 1,

  /**
   * @brief 处理数据完毕（无论正常异常），需要断开链接
   *
   */
  disconnect = 2,
};

/**
 * @brief 拆包器接口，使用者需要继承这个类，并且实现 data_received 方法
 *
 */
class base_packet_assemble {
public:

  /**
   * @brief 当tcp链接有数据读取时，会调用这个方法
   *
   * @param connection 读取到数据的 socket 链接
   * @param s 读取到的数据
   * @return data_read_result 数据的处理结果
   */
  virtual data_read_result
  data_received(std::shared_ptr<connection_handle> connection,
                std::string s) = 0;

  virtual ~base_packet_assemble() = default;
};

} // namespace salt