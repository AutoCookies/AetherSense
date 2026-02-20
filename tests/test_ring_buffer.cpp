#include <chrono>
#include <memory>

#include "test_harness.hpp"

#include "aethersense/runtime/ring_buffer.hpp"

TEST_CASE(RingBuffer_drop_oldest_policy) {
  aethersense::RingBuffer<int> ring(3);
  REQUIRE(ring.push(1, aethersense::BackpressurePolicy::kDropOldest));
  REQUIRE(ring.push(2, aethersense::BackpressurePolicy::kDropOldest));
  REQUIRE(ring.push(3, aethersense::BackpressurePolicy::kDropOldest));
  REQUIRE(ring.push(4, aethersense::BackpressurePolicy::kDropOldest));
  int out = 0;
  REQUIRE(ring.try_pop(out));
  REQUIRE(out == 2);
}

TEST_CASE(RingBuffer_drop_newest_policy) {
  aethersense::RingBuffer<int> ring(2);
  REQUIRE(ring.push(1, aethersense::BackpressurePolicy::kDropNewest));
  REQUIRE(ring.push(2, aethersense::BackpressurePolicy::kDropNewest));
  REQUIRE(!ring.push(3, aethersense::BackpressurePolicy::kDropNewest));
  int out = 0;
  REQUIRE(ring.try_pop(out));
  REQUIRE(out == 1);
  REQUIRE(ring.try_pop(out));
  REQUIRE(out == 2);
}

TEST_CASE(RingBuffer_block_policy_timeout_and_move_only) {
  aethersense::RingBuffer<std::unique_ptr<int>> ring(1);
  REQUIRE(ring.push(std::make_unique<int>(7), aethersense::BackpressurePolicy::kBlock));
  REQUIRE(!ring.push(std::make_unique<int>(8), aethersense::BackpressurePolicy::kBlock,
                     std::chrono::milliseconds(5)));
  std::unique_ptr<int> out;
  REQUIRE(ring.try_pop(out));
  REQUIRE(*out == 7);
}
