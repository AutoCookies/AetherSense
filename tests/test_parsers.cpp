#include "test_harness.hpp"

#include "aethersense/io/csi_reader.hpp"

TEST_CASE(CSV_parser_reads_frame_with_exact_size) {
  auto reader = aethersense::CreateReader("csv", "../testdata/csi_small.csv");
  REQUIRE(reader.ok());

  const auto frame = reader.value()->next();
  REQUIRE(frame.ok());
  REQUIRE(frame.value().has_value());
  REQUIRE(frame.value()->timestamp_ns == 1000);
  REQUIRE(frame.value()->sample_count() == 4);
}

TEST_CASE(JSONL_parser_reads_frame_with_exact_size) {
  auto reader = aethersense::CreateReader("jsonl", "../testdata/csi_small.jsonl");
  REQUIRE(reader.ok());

  const auto frame = reader.value()->next();
  REQUIRE(frame.ok());
  REQUIRE(frame.value().has_value());
  REQUIRE(frame.value()->center_freq_hz == 5800000000ULL);
  REQUIRE(frame.value()->sample_count() == 4);
}
