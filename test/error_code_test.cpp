#include "gtest/gtest.h"

#include <type_traits>

#include "salt/core/error.h"

TEST(salt_error_test, test_error_catetory) {
  auto error_code = salt::make_error_code(salt::error_code::success);
  ASSERT_EQ(error_code.category(), salt::error_category());
  ASSERT_EQ(error_code.value(),
            static_cast<std::underlying_type_t<salt::error_code>>(
                salt::error_code::success));
}