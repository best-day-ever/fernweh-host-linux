/**
 * @file tests/unit/test_capture_image_layout.cpp
 * @brief Verify capture metadata across the C++ and NVCC object boundary.
 */
#include "src/platform/capture_image.h"
#include "src/platform/linux/cuda.h"
#include <gtest/gtest.h>

#if defined(__linux__) && defined(SUNSHINE_BUILD_CUDA)
TEST(CudaCaptureImageLayout, RetentionHintDoesNotChangeCaptureTimestamp) {
  platf::img_t image;
  for (const bool preferred : {false, true}) {
    image.capture_pool_preferred = preferred;
    image.frame_timestamp.reset();
    EXPECT_FALSE(cuda::test_capture_image_timestamp(image).has_value());
    for (const std::int64_t ticks : {std::int64_t {0}, std::int64_t {0x12345678}}) {
      image.frame_timestamp = std::chrono::steady_clock::time_point {
        std::chrono::steady_clock::duration {ticks}
      };
      const auto observed = cuda::test_capture_image_timestamp(image);
      ASSERT_TRUE(observed.has_value());
      EXPECT_EQ(*observed, ticks);
    }
  }
}
#endif
