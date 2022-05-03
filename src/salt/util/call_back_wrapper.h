#pragma once

#include <utility>

namespace salt {

template <typename CallBackType, typename... Args>
inline void call(CallBackType &&call_back, Args &&...args) {
  if (!call_back)
    return;

  std::forward<CallBackType>(call_back)(std::forward<Args>(args)...);
}

} // namespace salt
