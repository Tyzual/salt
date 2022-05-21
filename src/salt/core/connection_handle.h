#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <system_error>

namespace salt {

/**
 * @brief socket 链接，通常作为系统回调的参数使用。不需要用户手动创建
 *
 */
class connection_handle {
public:
  /**
   * @brief 发送数据
   *
   * @param data 需要发送的数据
   * @param call_back
   * 发送数据完成的回调，可以从call_back的error_code参数得知是否发送成功
   */
  virtual void send(std::string data,
                    std::function<void(const std::error_code &)> call_back) = 0;
  virtual ~connection_handle() = default;
};

} // namespace salt