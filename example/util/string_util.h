#pragma once

#include <algorithm>
#include <cctype>
#include <locale>
#include <string>

namespace util {
namespace string {

/**
 * @brief 判断string是否已token开头
 */
inline bool start_with(const std::string &string, const std::string &token) {
  return string.rfind(token, 0) == 0;
}

inline void in_place_left_trim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void in_plcate_right_trim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline void in_place_trim(std::string &s) {
  in_place_left_trim(s);
  in_plcate_right_trim(s);
}

inline std::string left_trim(std::string s) {
  in_place_left_trim(s);
  return s;
}

inline std::string right_trim(std::string s) {
  in_plcate_right_trim(s);
  return s;
}

inline std::string trim(std::string s) {
  in_place_trim(s);
  return s;
}

} // namespace string
} // namespace util
