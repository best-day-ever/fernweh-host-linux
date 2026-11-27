/**
 * @file src/platform/linux/cursor_shape_retry.h
 * @brief Retry transiently unavailable XFixes cursor images without ending the stream.
 */
#pragma once

#include <chrono>
#include <optional>

namespace platf::x11 {
  /** @brief Outcome of updating the cursor shape. */
  enum class cursor_shape_result_e {
    active,  ///< The stream can continue.
    first_unavailable,  ///< The first miss in this retry sequence may be logged.
    delivery_failed,  ///< A captured shape could not be queued.
  };

  /** @brief Schedule the initial image and later shape changes independently of media. */
  class cursor_shape_retry_t {
  public:
    /**
     * @brief Try a due cursor shape and retain pending work across missing images.
     * @param now Current monotonic time.
     * @param changed Whether XFixes reported a shape change.
     * @param capture Return true when XFixes supplies an image.
     * @param publish Return true when the captured shape is accepted for delivery.
     * @return Whether the stream can continue and whether to report a failure.
     */
    template<typename Capture, typename Publish>
    cursor_shape_result_e update(std::chrono::steady_clock::time_point now, bool changed, Capture capture, Publish publish) {
      if (!next_attempt_ && changed) {
        next_attempt_ = fresh_attempt;
      }
      if (!next_attempt_ || now < *next_attempt_) {
        return cursor_shape_result_e::active;
      }
      if (!capture()) {
        const bool first_miss = *next_attempt_ == fresh_attempt;
        next_attempt_ = now + std::chrono::milliseconds(100);
        return first_miss ? cursor_shape_result_e::first_unavailable : cursor_shape_result_e::active;
      }
      next_attempt_.reset();
      return publish() ? cursor_shape_result_e::active : cursor_shape_result_e::delivery_failed;
    }

  private:
    static constexpr auto fresh_attempt = std::chrono::steady_clock::time_point::min();  ///< Due immediately with no prior miss.
    std::optional<std::chrono::steady_clock::time_point> next_attempt_ {fresh_attempt};  ///< Empty until another shape event.
  };
}
