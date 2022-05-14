#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include "salt/core/error.h"
#include "salt/core/log.h"
#include "salt/packet_assemble/packet_assemble.h"
#include "salt/util/byte_order.h"

namespace salt {

/**
 * @brief body长度的计算模式
 *
 * body_only          长度只包含body本身
 * with_length_field  长度包含body+长度字段
 * with_header          长度包含body+头
 */
enum class body_length_calc_mode {
  body_only = 1,
  with_length_field,
  with_header,
};

class header_body_assemble_notify {
public:
  virtual data_read_result
  packet_reserved(std::shared_ptr<connection_handle> connection,
                  std::string header, std::string body) = 0;

  virtual data_read_result
  header_read_finish(std::shared_ptr<connection_handle> connection,
                   const std::string &header) {
    return data_read_result::success;
  }

  virtual void packet_read_error(const std::error_code &error_code) {}

  ~header_body_assemble_notify() = default;
};

template <typename header_type, auto length_property>
class header_body_assemble : public base_packet_assemble {
public:
  static_assert(std::is_standard_layout_v<header_type>,
                "header is not standard layout");
  static_assert(std::has_unique_object_representations_v<header_type>,
                "header has padding");

  body_length_calc_mode body_length_calc_mode_{
      body_length_calc_mode::body_only};

  uint32_t body_length_limit_{0};

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
  std::unique_ptr<header_body_assemble_notify> notify_{nullptr};
};

///////////////////////////////////////////////////////////////////////////////////////////
// 模板函数实现
///////////////////////////////////////////////////////////////////////////////////////////

template <typename header_type, auto length_property>
data_read_result header_body_assemble<header_type, length_property>::data_received(
    std::shared_ptr<connection_handle> connection, std::string s) {

  auto check_size = [this]<typename calc_type>() {
    if (body_size_ <= sizeof(calc_type)) {
      if (notify_) {
        notify_->packet_read_error(
            make_error_code(error_code::body_size_error));
      }
      log_error("body size error, receive body len:%llu, lengh type size:%zu",
                body_size_, sizeof(calc_type));
      return data_read_result::disconnect;
    } else {
      body_size_ -= sizeof(calc_type);
      return data_read_result::success;
    }
  };

  auto calc_body_size = [&] {
    switch (body_length_calc_mode_) {
    case body_length_calc_mode::with_length_field: {
      auto result = check_size.template operator()<size_type>();
      if (result != data_read_result::success) {
        return result;
      }
    } break;
    case body_length_calc_mode::with_header: {
      auto result = check_size.template operator()<header_type>();
      if (result != data_read_result::success) {
        return result;
      }
    } break;
    default: {
    } break;
    };

    if (body_length_limit_ != 0 && body_size_ > body_length_limit_) {
      if (notify_) {
        notify_->packet_read_error(
            make_error_code(error_code::body_size_error));
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
        body_size_ =
            reinterpret_cast<header_type *>(header_.data())->*length_property;
        body_size_ = byte_order::to_host(body_size_);
        auto result = calc_body_size();
        if (result != data_read_result::success) {
          return result;
        }
        rest_length_ = body_size_;
        current_stat_ = parse_stat::body;
        return data_read_result::success;
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
        if (notify_) {
          auto result = notify_->header_read_finish(connection, header_);
          if (result != data_read_result::success) {
            if (notify_) {
              notify_->packet_read_error(
                  make_error_code(error_code::header_read_error));
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
        rest_length_ = body_size_ - rest_data_length;
        current_stat_ = parse_stat::body;
        return data_read_result::success;
      } else if (rest_data_length == rest_length_) {
        body_ += s.substr(offset);
        // TODO 通知上游，收到新包
        log_debug("get message body size:%llu, content:%s", body_size_,
                  body_.c_str());
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
        // TODO 通知上游，收到新包
        log_debug("get message body size:%llu, content:%s", body_size_,
                  body_.c_str());
        current_stat_ = parse_stat::header;
        rest_length_ = header_size_;
        header_.clear();
        body_.clear();
        body_size_ = 0;
      }
    } break;
    }
  }

  return data_read_result::disconnect;
}

} // namespace salt