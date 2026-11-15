/**
 * @file src/platform/capture_image.h
 * @brief Shared capture-image layout for C++ and CUDA translation units.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

namespace platf {
  /** @brief Captured frame buffer shared between capture and encode stages. */
  struct img_t: std::enable_shared_from_this<img_t> {
  public:
    img_t() = default;

    img_t(img_t &&) = delete;
    img_t(const img_t &) = delete;
    img_t &operator=(img_t &&) = delete;
    img_t &operator=(const img_t &) = delete;

    std::uint8_t *data {};  ///< Pointer to the captured image buffer.
    std::int32_t width {};  ///< Image width in pixels.
    std::int32_t height {};  ///< Image height in pixels.
    std::int32_t pixel_pitch {};  ///< Bytes per pixel in the image buffer.
    std::int32_t row_pitch {};  ///< Bytes between consecutive image rows.
    bool capture_pool_preferred {false};  ///< Immutable hint to retain costly registered buffers during idle trimming.

    std::optional<std::chrono::steady_clock::time_point> frame_timestamp;  ///< Capture timestamp associated with the frame.

    /** @brief Destroy the image after its final capture or encoder owner. */
    virtual ~img_t() = default;
  };
}  // namespace platf
