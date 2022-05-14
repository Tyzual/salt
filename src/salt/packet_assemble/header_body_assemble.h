#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "salt/core/error.h"
#include "salt/core/log.h"
#include "salt/packet_assemble/packet_assemble.h"
#include "salt/util/byte_order.h"

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
   *        使用这种模式会忽略
   *        header_body_assemble::reserve_body_size_
   *        字段
   */
  with_length_field,

  /**
   * @brief 长度包含body + sizeof(包头)
   *        使用这种模式会忽略
   *        header_body_assemble::reserve_body_size_
   *        字段
   */
  with_header,

  /**
   * @brief 长度包含body + 自定义长度
   *        自定义长度由 header_body_assemble::reserve_body_size_ 指定
   */
  custom_length,
};

template <typename _header_type> class header_body_assemble_notify {
public:
  using header_type = _header_type;
  virtual data_read_result
  packet_reserved(std::shared_ptr<connection_handle> connection,
                  std::string raw_header_data, std::string body) = 0;

  virtual data_read_result
  header_read_finish(std::shared_ptr<connection_handle> connection,
                     const header_type &header) {
    return data_read_result::success;
  }

  virtual void packet_read_error(const std::error_code &error_code,
                                 const std::string &message) {}

  inline header_type &to_header(std::string &raw_header_data) {
    return *reinterpret_cast<header_type *>(raw_header_data.data());
  }

  ~header_body_assemble_notify() = default;
};

template <typename header_type, auto length_property>
class header_body_assemble final : public base_packet_assemble {
public:
  static_assert(std::is_standard_layout_v<header_type>,
                "header is not standard layout");
  static_assert(std::has_unique_object_representations_v<header_type>,
                "header has padding");

  body_length_calc_mode body_length_calc_mode_{
      body_length_calc_mode::body_only};

  uint32_t reserve_body_size_{0};
  uint32_t body_length_limit_{0};

  inline void
  set_notify(std::unique_ptr<header_body_assemble_notify<header_type>> notify) {
    notify_ = std::move(notify);
  }

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
          auto result = notify_->header_read_finish(
              connection,
              *reinterpret_cast<const header_type *>(header_.data()));
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

        /**
         * 如果body_size_为0
         * 说明这是个空包
         * 这里需要走一个读body的流程
         * 将状态重置为读header
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
          auto result = notify_->header_read_finish(
              connection,
              *reinterpret_cast<const header_type *>(header_.data()));
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