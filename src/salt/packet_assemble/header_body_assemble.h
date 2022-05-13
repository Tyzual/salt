#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include "salt/core/log.h"
#include "salt/packet_assemble/packet_assemble.h"

namespace salt {

template <typename HeaderType, typename SizeType,
          SizeType HeaderType::*length_property>
class header_body_assemble : public base_packet_assemble {
public:
  static_assert(std::is_trivial_v<HeaderType>, "Header is not trivial");
  static_assert(std::is_standard_layout_v<HeaderType>,
                "Header is not standard layout");
  static_assert(std::has_unique_object_representations_v<HeaderType>,
                "Header has padding");

  data_read_result data_received(std::shared_ptr<connection_handle> connection,
                                 std::string s) override {

    uint32_t offset{0};
    auto rest_data_length=s.size();
    while (offset < s.size()) {
      switch (current_stat_) {
      case ParseStat::kHeader: {
        if (rest_length_ > rest_data_length) {
          rest_length_ -= rest_data_length;
          header_ += std::move(s);
          return data_read_result::disconnect;
        } else if (rest_length_ == rest_data_length) {
          header_ += std::move(s);
          body_size_ =
              reinterpret_cast<HeaderType *>(header_.data())->*length_property;
          rest_length_ = body_size_;
          current_stat_ = ParseStat::kBody;
          return data_read_result::disconnect;
        } else /* if (rest_length_ < rest_data_length) */ {
          rest_data_length -= rest_length_;
          header_ += s.substr(offset, rest_length_);
          offset += rest_length_;
          body_size_ =
              reinterpret_cast<HeaderType *>(header_.data())->*length_property;
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
          return data_read_result::disconnect;
        } else if (rest_data_length == rest_length_) {
          body_ += s.substr(offset);
          // TODO 通知上游，收到新包
          log_debug("get message body size:%llu, content:%s", body_size_, body_.c_str());
          current_stat_ = ParseStat::kHeader;
          rest_length_ = header_size_;
          header_.clear();
          body_.clear();
          return data_read_result::disconnect;
        } else /* if (rest_data_length > rest_length_) */ {
          rest_data_length -= rest_length_;
          body_ += s.substr(offset, rest_length_);
          offset += rest_length_;
          // TODO 通知上游，收到新包
          log_debug("get message body size:%llu, content:%s", body_size_, body_.c_str());
          current_stat_ = ParseStat::kHeader;
          rest_length_ = header_size_;
          header_.clear();
          body_.clear();
        }
      }
      }
    }

    return data_read_result::disconnect;
  }

  void _test(void *buff) {
    std::cout << reinterpret_cast<HeaderType *>(buff)->*length_property
              << std::endl;
  }

  ~header_body_assemble() = default;

private:
  std::string header_;
  std::string body_;
  static constexpr uint32_t header_size_ = sizeof(HeaderType);
  SizeType body_size_{0};

  enum class ParseStat {
    kHeader,
    kBody,
  };

  ParseStat current_stat_{ParseStat::kHeader};
  uint32_t rest_length_{header_size_};
};

} // namespace salt