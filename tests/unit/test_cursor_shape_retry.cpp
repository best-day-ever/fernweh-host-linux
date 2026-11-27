/**
 * @file tests/unit/test_cursor_shape_retry.cpp
 * @brief Cover transient XFixes image loss during stream startup and display changes.
 */
#include <gtest/gtest.h>
#include <src/platform/linux/cursor_shape_retry.h>

using namespace std::chrono_literals;
using platf::x11::cursor_shape_result_e;
using platf::x11::cursor_shape_retry_t;

TEST(CursorShapeRetry, RecoversTheInitialImageWithoutAnotherShapeEvent) {
  cursor_shape_retry_t retry;
  const std::chrono::steady_clock::time_point start {};
  int captures = 0;
  int deliveries = 0;
  const auto capture = [&] { return ++captures == 3; };
  const auto publish = [&] { return ++deliveries == 1; };

  EXPECT_EQ(retry.update(start, false, capture, publish), cursor_shape_result_e::first_unavailable);
  EXPECT_EQ(retry.update(start + 99ms, true, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(captures, 1);
  EXPECT_EQ(retry.update(start + 100ms, false, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(retry.update(start + 199ms, false, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(captures, 2);
  EXPECT_EQ(deliveries, 0);
  EXPECT_EQ(retry.update(start + 200ms, false, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(retry.update(start + 500ms, false, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(captures, 3);
  EXPECT_EQ(deliveries, 1);
}

TEST(CursorShapeRetry, RecoversAnUnavailableShapeAfterStreamingStarts) {
  cursor_shape_retry_t retry;
  const std::chrono::steady_clock::time_point start {};
  int captures = 0;
  int deliveries = 0;
  const auto capture = [&] { return ++captures != 2; };
  const auto publish = [&] {
    ++deliveries;
    return true;
  };

  EXPECT_EQ(retry.update(start, false, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(retry.update(start + 1s, true, capture, publish), cursor_shape_result_e::first_unavailable);
  EXPECT_EQ(deliveries, 1);
  EXPECT_EQ(retry.update(start + 1100ms, false, capture, publish), cursor_shape_result_e::active);
  EXPECT_EQ(deliveries, 2);
}

TEST(CursorShapeRetry, ReportsAFailedDeliveryOfACapturedShape) {
  cursor_shape_retry_t retry;
  const std::chrono::steady_clock::time_point start {};

  EXPECT_EQ(retry.update(start, false, [] { return true; }, [] { return false; }),
            cursor_shape_result_e::delivery_failed);
}
