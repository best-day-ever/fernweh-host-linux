/**
 * @file src/platform/linux/nvfbc_capture_timing.h
 * @brief Separate driver frame cadence from the NvFBC polling schedule.
 */
#pragma once

#include "src/frame_timing.h"

namespace nvfbc {
  /** @brief Collect bounded timing data for one capture loop. */
  struct capture_timing_t {
    /**
     * @brief Record metadata from a successful NvFBC grab.
     * @param fresh Whether the driver reported a new frame.
     * @param timestamp_us The display server's render timestamp, or zero when unavailable.
     * @param missed The driver's missed-frame counter for this grab.
     */
    void record_grab(bool fresh, std::uint64_t timestamp_us, std::uint32_t missed) {
      if (!fresh) {
        ++repeated_grabs;
        return;
      }
      ++fresh_grabs;
      driver_reported_missed_frames += missed;
      if (previous_timestamp_ && timestamp_us > previous_timestamp_) {
        source_interval_us.push_back(static_cast<double>(timestamp_us - previous_timestamp_));
      }
      previous_timestamp_ = timestamp_us;
    }

    /** @brief Keep totals but discard the timestamp when a driver session is recreated. */
    void begin_driver_session() { previous_timestamp_ = 0; }

    video::timing_samples_t poll_lateness_us;  ///< Time from scheduled poll to invocation.
    video::timing_samples_t source_interval_us;  ///< Time between fresh timestamps observed in capture.
    std::uint64_t fresh_grabs {};  ///< Successful grabs marked fresh by the driver.
    std::uint64_t repeated_grabs {};  ///< Successful grabs returning an earlier frame.
    std::uint64_t driver_reported_missed_frames {};  ///< Sum reported on fresh grabs.

  private:
    std::uint64_t previous_timestamp_ {};  ///< Most recent fresh source timestamp.
  };
}
