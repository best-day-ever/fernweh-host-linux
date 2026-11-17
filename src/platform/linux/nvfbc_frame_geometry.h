/**
 * @file src/platform/linux/nvfbc_frame_geometry.h
 * @brief Validate NvFBC BGRA buffers before copying their pixels.
 */
#pragma once

#include <cstdint>
#include <limits>

namespace nvfbc {
  /**
   * @brief Check that a grabbed BGRA frame has the capture's expected layout.
   *
   * NvFBC owns the source buffer and can replace it during a modeset. Its
   * reported dimensions, not the destination texture's pitch, describe that
   * buffer. Reject changes before copying or suppressing a duplicate frame.
   * @param width Width reported by the successful grab.
   * @param height Height reported by the successful grab.
   * @param bytes Available source bytes reported by the driver.
   * @param expected_width Width selected when the capture was initialized.
   * @param expected_height Height selected when the capture was initialized.
   * @return True if the complete packed BGRA image is safe to copy.
   */
  inline bool frame_matches_bgra(std::uint32_t width, std::uint32_t height, std::uint32_t bytes,
                                int expected_width, int expected_height) {
    if (expected_width <= 0 || expected_height <= 0 ||
        width != static_cast<std::uint32_t>(expected_width) ||
        height != static_cast<std::uint32_t>(expected_height) ||
        width > static_cast<std::uint32_t>(std::numeric_limits<int>::max() / 4)) {
      return false;
    }
    // A row pitch is an int in tex_t; do the complete size calculation in
    // uint64_t so malformed driver metadata cannot wrap a smaller allocation.
    return static_cast<std::uint64_t>(width) * height * 4U <= bytes;
  }
}
