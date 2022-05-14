#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include "salt/core/log.h"
#include "salt/packet_assemble/packet_assemble.h"

namespace salt {

template <typename header_type, typename size_type,
          size_type header_type::*length_property>
class header_body_assemble : public base_packet_assemble {
public:
  static_assert(std::is_trivial_v<header_type>, "Header is not trivial");
  static_assert(std::is_standard_layout_v<header_type>,
                "Header is not standard layout");
  static_assert(std::has_unique_object_representations_v<header_type>,
                "Header has padding");

  data_read_result data_received(std::shared_ptr<connection_handle> connection,
                                 std::string s) override;

  ~header_body_assemble() = default;

private:
  std::string header_;
  std::string body_;
  static constexpr uint32_t header_size_ = sizeof(header_type);
  size_type body_size_{0};

  enum class ParseStat {
    kHeader,
    kBody,
  };

  ParseStat current_stat_{ParseStat::kHeader};
  uint32_t rest_length_{header_size_};
};

///////////////////////////////////////////////////////////////////////////////////////////
// 模板函数实现
///////////////////////////////////////////////////////////////////////////////////////////

template <typename header_type, typename size_type,
          size_type header_type::*length_property>
data_read_result
header_body_assemble<header_type, size_type, length_property>::data_received(
    std::shared_ptr<connection_handle> connection, std::string s) {
  uint32_t offset{0};
  auto rest_data_length = s.size();
  while (offset < s.size()) {
    switch (current_stat_) {
    case ParseStat::kHeader: {
      if (rest_length_ > rest_data_length) {
        rest_length_ -= rest_data_length;
        header_ += s.substr(offset);
        return data_read_result::success;
      } else if (rest_length_ == rest_data_length) {
        header_ += std::move(s);
        body_size_ =
            reinterpret_cast<header_type *>(header_.data())->*length_property;
        rest_length_ = body_size_;
        current_stat_ = ParseStat::kBody;
        return data_read_result::success;
      } else /* if (rest_length_ < rest_data_length) */ {
        rest_data_length -= rest_length_;
        header_ += s.substr(offset, rest_length_);
        offset += rest_length_;
        body_size_ =
            reinterpret_cast<header_type *>(header_.data())->*length_property;
        body_.clear();
        body_.reserve(body_size_);
        rest_length_ = body_size_;
        current_stat_ = ParseStat::kBody;
      }
    }
      [[fallthrough]];
    case ParseStat::kBody: {
      if (rest_data_length < rest_length_) {
        body_ += s.substr(offset);
        rest_length_ = body_size_ - rest_data_length;
        current_stat_ = ParseStat::kBody;
        return data_read_result::success;
      } else if (rest_data_length == rest_length_) {
        body_ += s.substr(offset);
        // TODO 通知上游，收到新包
        log_debug("get message body size:%llu, content:%s", body_size_,
                  body_.c_str());
        current_stat_ = ParseStat::kHeader;
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
        current_stat_ = ParseStat::kHeader;
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