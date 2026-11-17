/**
 * @file tests/unit/test_nvfbc_frame_geometry.cpp
 * @brief Reject resized or incomplete capture buffers before CUDA copies.
 */
#include <gtest/gtest.h>
#include <src/platform/linux/nvfbc_frame_gate.h>
#include <src/platform/linux/nvfbc_frame_geometry.h>

#include <limits>

TEST(NvfbcFrameGeometry, AcceptsCompleteBgraFrameAndExtraAllocationCapacity) {
  EXPECT_TRUE(nvfbc::frame_matches_bgra(1920, 1080, 1920U * 1080U * 4U, 1920, 1080));
  EXPECT_TRUE(nvfbc::frame_matches_bgra(1920, 1080, 1920U * 1080U * 4U + 4096U, 1920, 1080));
}

TEST(NvfbcFrameGeometry, RejectsSmallerModesetBeforeReadingPastDriverBuffer) {
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1920, 1080, 1920U * 1080U * 4U, 3024, 1898));
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1512, 949, 1512U * 949U * 4U, 1920, 1080));
}

TEST(NvfbcFrameGeometry, RejectsLargerFrameInsteadOfInterpretingRowsWithOldPitch) {
  EXPECT_FALSE(nvfbc::frame_matches_bgra(3024, 1898, 3024U * 1898U * 4U, 1920, 1080));
  // Equal byte counts do not imply equal strides or geometry.
  EXPECT_FALSE(nvfbc::frame_matches_bgra(2160, 960, 1920U * 1080U * 4U, 1920, 1080));
}

TEST(NvfbcFrameGeometry, RejectsTruncatedBgraStorageAndInvalidDimensions) {
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1920, 1080, 1920U * 1080U * 4U - 1U, 1920, 1080));
  EXPECT_FALSE(nvfbc::frame_matches_bgra(0, 1080, 0, 0, 1080));
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1920, 0, 0, 1920, 0));
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1920, 1080, 1920U * 1080U * 4U, -1, 1080));
}

TEST(NvfbcFrameGeometry, RejectsOverflowingRowPitchAndImageSize) {
  const auto maximum = std::numeric_limits<std::uint32_t>::max();
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1U << 29, 2, maximum, 1 << 29, 2));
  EXPECT_FALSE(nvfbc::frame_matches_bgra(32768, 32768, maximum, 32768, 32768));
}

TEST(NvfbcFrameGeometry, PackedCaptureUsesWholeScreenNotPackedDestination) {
  EXPECT_TRUE(nvfbc::frame_matches_bgra(3840, 1080, 3840U * 1080U * 4U, 3840, 1080));
  EXPECT_FALSE(nvfbc::frame_matches_bgra(1920, 2160, 3840U * 1080U * 4U, 3840, 1080));
}

TEST(NvfbcFrameGeometry, ModesetIsRejectedEvenWhenDriverCallsItADuplicate) {
  nvfbc::frame_gate_t gate;
  const auto publish = [&](std::uint32_t width, std::uint32_t height, bool changed) {
    return nvfbc::frame_matches_bgra(width, height, width * height * 4U, 1920, 1080) && gate.publish(changed);
  };
  EXPECT_TRUE(publish(1920, 1080, false));
  EXPECT_FALSE(publish(1920, 1080, false));
  EXPECT_FALSE(publish(1512, 949, false));
  gate.reset();
  EXPECT_FALSE(publish(1512, 949, false));
  EXPECT_TRUE(publish(1920, 1080, false));
}
