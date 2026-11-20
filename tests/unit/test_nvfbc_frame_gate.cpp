/**
 * @file tests/unit/test_nvfbc_frame_gate.cpp
 * @brief Cover duplicate suppression and capture-session recreation.
 */
#include <gtest/gtest.h>
#include <src/platform/linux/nvfbc_capture_timing.h>
#include <src/platform/linux/nvfbc_frame_gate.h>

TEST(NvfbcFrameGate, PublishesInitialImageAndChangesButSkipsDuplicates) {
  nvfbc::frame_gate_t gate;
  EXPECT_TRUE(gate.publish(false));
  EXPECT_FALSE(gate.publish(false));
  EXPECT_FALSE(gate.publish(false));
  EXPECT_TRUE(gate.publish(true));
  EXPECT_FALSE(gate.publish(false));
  gate.reset();
  EXPECT_TRUE(gate.publish(false));
  EXPECT_FALSE(gate.publish(false));
}

TEST(NvfbcCaptureTiming, SeparatesRepeatedGrabsMissingFramesAndDriverRestarts) {
  nvfbc::capture_timing_t timing;
  timing.record_grab(true, 100000, 0);
  timing.record_grab(false, 100000, 7);
  timing.record_grab(true, 133334, 1);
  timing.record_grab(true, 0, 0);
  timing.record_grab(true, 150000, 0);
  timing.record_grab(true, 140000, 0);
  timing.begin_driver_session();
  timing.record_grab(true, 1000000, 0);
  timing.record_grab(true, 1016667, 0);

  EXPECT_EQ(timing.fresh_grabs, 7u);
  EXPECT_EQ(timing.repeated_grabs, 1u);
  EXPECT_EQ(timing.driver_reported_missed_frames, 1u);
  const auto interval = timing.source_interval_us.summary();
  EXPECT_EQ(interval.count, 2u);
  EXPECT_EQ(interval.p50, 16667);
  EXPECT_EQ(interval.maximum, 33334);
}
