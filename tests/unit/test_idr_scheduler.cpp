#include "src/nvenc/idr_scheduler.h"
#include <gtest/gtest.h>

TEST(NvencIdrScheduler, RetainsRequestsUntilTheAllowedFrame) {
  using namespace std::chrono_literals;
  nvenc::idr_scheduler scheduler;
  const auto start = nvenc::idr_scheduler::clock::time_point {};
  EXPECT_FALSE(scheduler.take_request(start));
  scheduler.request();
  EXPECT_TRUE(scheduler.take_request(start));
  scheduler.request();
  EXPECT_FALSE(scheduler.take_request(start + 200ms));
  scheduler.request(); // Coalescing must not postpone the original deadline.
  EXPECT_FALSE(scheduler.take_request(start + 999ms));
  EXPECT_TRUE(scheduler.take_request(start + 1000ms));
  EXPECT_FALSE(scheduler.take_request(start + 2000ms));
  scheduler.request();
  EXPECT_TRUE(scheduler.take_request(start + 2000ms));
}
