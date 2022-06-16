#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "salt/core/error.h"
#include "salt/core/log.h"
#include "salt/packet_assemble/assemble_conf.h"
#include "salt/packet_assemble/packet_assemble.h"
#include "salt/util/byte_order.h"

namespace salt {

/**
 * @brief
 * 拆完包之后的回调，用户需要继承这个类，并且实现对应的方法来获取拆完包以后的内容
 *
 * @tparam header_type 包头类型
 */
template <typename header_type> class header_body_assemble_notify {
public:
  /**
   * @brief 解完整个包以后的回调
   *
   * @param connection 收到包的链接，可以使用这个参数发回包
   * @param raw_header_data 包头部原始数据
   * @param body 包内容原始数据
   * @return data_read_result 处理包的结果
   */
  virtual data_read_result
  packet_reserved(std::shared_ptr<connection_handle> connection,
                  std::string raw_header_data, std::string body) = 0;

  /**
   * @brief 仅解析完头部以后的回调。
   *        如果有需要可以 override 这个接口来验证头部是否合法
   *
   * @param connection 收到包的链接，可以使用这个参数发回包
   * @param raw_header_data 头部的原始数据
   * @return data_read_result 处理包头的结果
   */
  virtual data_read_result
  header_read_finish(std::shared_ptr<connection_handle> connection,
                     const std::string &raw_header_data) {
    return data_read_result::success;
  }

  /**
   * @brief 拆包错误时的回调，当 salt
   * 解析到不合法的包（比如长度超过最大限制的包）时，会调用此回调
   *
   * @param error_code 错误码
   * @param message 额外的错误信息
   */
  virtual void packet_read_error(const std::error_code &error_code,
                                 const std::string &message) {}

  /**
   * @brief 将原始包头数据转换成包头的结构。
   *        转换完成以后，包头中各个数据的字节序仍然是网络字节序
   *
   * @param raw_header_data 包头原始数据
   * @return header_type& 包头的数据结构
   */
  inline header_type &to_header(std::string &raw_header_data) {
    return *reinterpret_cast<header_type *>(raw_header_data.data());
  }

  /**
   * @brief 将原始包头数据转换成包头的结构。
   *        转换完成以后，包头中各个数据的字节序仍然是网络字节序
   *
   * @param raw_header_data 包头原始数据
   * @return header_type& 包头的数据结构
   */
  inline const header_type &
  to_header(const std::string &raw_header_data) const {
    return *reinterpret_cast<const header_type *>(raw_header_data.data());
  }

  virtual ~header_body_assemble_notify() = default;
};

/**
 * @brief 基于包头和包内容的拆包器
 *        使用方法可以查看example/message/tcp_message_client.cpp
 *
 * @tparam header_type 包头的类型，包头需要满足两条件 1.包头为
 * POD； 2.包头各成员之间无 padding。虽然 salt
 * 会尝试检测以上两个条件是否满足，但是不能保证一定可靠。所以，以上两个条件需要用户自行保证
 *
 * @tparam length_property 包头中表示包内容长度的字段
 */
template <typename header_type, auto length_property>
class header_body_assemble final : public base_packet_assemble {
public:
  static_assert(std::is_standard_layout_v<header_type>,
                "header is not standard layout");
  static_assert(std::has_unique_object_representations_v<header_type>,
                "header has padding");

  /**
   * @brief 包内容长度的计算方式
   *
   */
  body_length_calc_mode body_length_calc_mode_{
      body_length_calc_mode::body_only};

  /**
   * @brief 包内容长度的保留字段，使用方法见
   * body_length_calc_mode::custom_length
   *
   */
  uint32_t reserve_body_size_{0};

  /**
   * @brief
   * 最大包内容长度限制，如果此字段不为0，在解析到超过限制的包内容长度后，salt
   * 会断开链接
   *
   */
  uint32_t body_length_limit_{0};

  /**
   * @brief 设置拆完包以后的回调
   *
   * @param notify 拆包完成后的回调
   */
  inline void
  set_notify(std::unique_ptr<header_body_assemble_notify<header_type>> notify) {
    notify_ = std::move(notify);
  }

  /**
   * @brief socket 链接读取到数据时的回调，在使用这个类时，用户不需要关注此方法
   *
   * @param connection 读取到数据的链接
   * @param s 读取到的数据
   * @return data_read_result 处理数据的结果
   */
  data_read_result data_received(std::shared_ptr<connection_handle> connection,
                                 std::string s) override final;

  ~header_body_assemble() = default;

private:
  std::string header_;
  std::string body_;
  static constexpr uint32_t header_size_ = sizeof(header_type);
  using size_type = decltype(std::declval<header_type>().*length_property);
  size_type body_size_{0};

  enum class parse_stat {
    header,
    body,
  };

  parse_stat current_stat_{parse_stat::header};
  uint32_t rest_length_{header_size_};
  std::unique_ptr<header_body_assemble_notify<header_type>> notify_{nullptr};
};

///////////////////////////////////////////////////////////////////////////////////////////
// 模板函数实现
///////////////////////////////////////////////////////////////////////////////////////////

template <typename header_type, auto length_property>
data_read_result
header_body_assemble<header_type, length_property>::data_received(
    std::shared_ptr<connection_handle> connection, std::string s) {

  auto check_size = [this](uint32_t reserve_size) {
    if (body_size_ < reserve_size) {
      if (notify_) {
        std::stringstream ss;
        ss << "body size error. body should greater than:" << reserve_size
           << ", receive:" << body_size_;
        notify_->packet_read_error(make_error_code(error_code::body_size_error),
                                   std::move(ss).str());
      }
      log_error("body size error, receive body len:%llu, lengh type size:%u",
                body_size_, reserve_size);
      return data_read_result::disconnect;
    } else {
      body_size_ -= reserve_size;
      return data_read_result::success;
    }
  };

  auto calc_body_size = [&] {
    log_debug("raw body length:%u", body_size_);
    switch (body_length_calc_mode_) {
    case body_length_calc_mode::with_length_field: {
      auto result = check_size(sizeof(size_type));
      if (result != data_read_result::success) {
        return result;
      }
    } break;
    case body_length_calc_mode::with_header: {
      auto result = check_size(sizeof(header_type));
      if (result != data_read_result::success) {
        return result;
      }
    } break;
    case body_length_calc_mode::custom_length: {
      auto result = check_size(reserve_body_size_);
      if (result != data_read_result::success) {
        return result;
      }
    } break;
    default: {
    } break;
    };

    if (body_length_limit_ != 0 && body_size_ > body_length_limit_) {
      if (notify_) {
        std::stringstream ss;
        ss << "body size:" << body_size_
           << " exceeds limit:" << body_length_limit_;
        notify_->packet_read_error(make_error_code(error_code::body_size_error),
                                   std::move(ss).str());
      }
      log_error("body size:%llu exceeds limit:%u", body_size_,
                body_length_limit_);
      return data_read_result::disconnect;
    }

    return data_read_result::success;
  };

  uint32_t offset{0};
  auto rest_data_length = s.size();
  while (offset < s.size()) {
    switch (current_stat_) {
    case parse_stat::header: {
      if (rest_length_ > rest_data_length) {
        rest_length_ -= rest_data_length;
        header_ += s.substr(offset);
        current_stat_ = parse_stat::header;
        return data_read_result::success;
      } else if (rest_length_ == rest_data_length) {
        header_ += s.substr(offset);
        rest_data_length -= rest_length_;
        offset += rest_length_;
        body_size_ =
            reinterpret_cast<header_type *>(header_.data())->*length_property;
        body_size_ = byte_order::to_host(body_size_);
        auto result = calc_body_size();
        if (result != data_read_result::success) {
          return result;
        }
        if (header_.size() != sizeof(header_type)) {
          if (notify_) {
            notify_->packet_read_error(
                make_error_code(error_code::internel_error),
                error_category().message(
                    static_cast<std::underlying_type_t<error_code>>(
                        error_code::internel_error)));
          }
          log_error("read header error, header size %zu, expected:%u",
                    sizeof(header_type), header_.size());
          return data_read_result::disconnect;
        }
        if (notify_) {
          auto result = notify_->header_read_finish(connection, header_);
          if (result != data_read_result::success) {
            if (notify_) {
              notify_->packet_read_error(
                  make_error_code(error_code::header_read_error),
                  "notify return error");
              return result;
            }
          }
        }
        rest_length_ = body_size_;
        current_stat_ = parse_stat::body;

        /** <!-- 让 doxygen 忽略这段话
         * 如果body_size_为0
         * 说明这是个空包
         * 这里需要走一个读body的流程
         * 将状态重置为读header
         * -->
         */
        if (body_size_ != 0) {
          return data_read_result::success;
        }
      } else /* if (rest_length_ < rest_data_length) */ {
        rest_data_length -= rest_length_;
        header_ += s.substr(offset, rest_length_);
        offset += rest_length_;
        body_size_ =
            reinterpret_cast<header_type *>(header_.data())->*length_property;
        body_size_ = byte_order::to_host(body_size_);
        auto result = calc_body_size();
        if (result != data_read_result::success) {
          return result;
        }
        if (header_.size() != sizeof(header_type)) {
          if (notify_) {
            notify_->packet_read_error(
                make_error_code(error_code::internel_error),
                error_category().message(
                    static_cast<std::underlying_type_t<error_code>>(
                        error_code::internel_error)));
          }
          log_error("read header error, header size %zu, expected:%u",
                    sizeof(header_type), header_.size());
          return data_read_result::disconnect;
        }
        if (notify_) {
          auto result = notify_->header_read_finish(connection, header_);
          if (result != data_read_result::success) {
            if (notify_) {
              notify_->packet_read_error(
                  make_error_code(error_code::header_read_error),
                  "notify return error");
              return result;
            }
          }
        }
        body_.clear();
        body_.reserve(body_size_);
        rest_length_ = body_size_;
        current_stat_ = parse_stat::body;
      }
    }
      [[fallthrough]];
    case parse_stat::body: {
      if (rest_data_length < rest_length_) {
        body_ += s.substr(offset);
        rest_length_ -= rest_data_length;
        current_stat_ = parse_stat::body;
        return data_read_result::success;
      } else if (rest_data_length == rest_length_) {
        body_ += s.substr(offset);
        log_debug("get message body size:%llu, content:%s", body_size_,
                  body_.c_str());
        if (notify_) {
          notify_->packet_reserved(connection, std::move(header_),
                                   std::move(body_));
        }
        current_stat_ = parse_stat::header;
        rest_length_ = header_size_;
        header_.clear();
        body_.clear();
        body_size_ = 0;
        return data_read_result::success;
      } else /* if (rest_data_length > rest_length_) */ {
        rest_data_length -= rest_length_;
        body_ += s.substr(offset, rest_length_);
        offset += rest_length_;
        log_debug("get message body size:%llu, content:%s", body_size_,
                  body_.c_str());
        if (notify_) {
          notify_->packet_reserved(connection, std::move(header_),
                                   std::move(body_));
        }
        current_stat_ = parse_stat::header;
        rest_length_ = header_size_;
        header_.clear();
        body_.clear();
        body_size_ = 0;
      }
    } break;
    }
  }

  return data_read_result::success;
}

} // namespace salt