#include <chrono>
#include <memory>

#include "test_harness.hpp"

#include "aethersense/runtime/ring_buffer.hpp"

TEST_CASE(RingBuffer_wraparound_keeps_latest_entries) {
  aethersense::RingBuffer<int> ring(3);
  ring.push(1);
  ring.push(2);
  ring.push(3);
  ring.push(4);

  int out = 0;
  REQUIRE(ring.try_pop(out));
  REQUIRE(out == 2);
  REQUIRE(ring.try_pop(out));
  REQUIRE(out == 3);
  REQUIRE(ring.try_pop(out));
  REQUIRE(out == 4);
}

TEST_CASE(RingBuffer_timeout_is_deterministic) {
  aethersense::RingBuffer<int> ring(8);
  const auto result = ring.pop_blocking(std::chrono::milliseconds(20));
  REQUIRE(!result.ok());
  REQUIRE(result.error().code == aethersense::ErrorCode::kTimeout);
}

TEST_CASE(RingBuffer_supports_move_only_types) {
  aethersense::RingBuffer<std::unique_ptr<int>> ring(8);
  ring.push(std::make_unique<int>(42));
  std::unique_ptr<int> out;
  REQUIRE(ring.try_pop(out));
  REQUIRE(*out == 42);
}
