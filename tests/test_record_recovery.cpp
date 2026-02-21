#include "test_harness.hpp"

#include "aethersense/io/record_recovery.hpp"

TEST_CASE(Record_recovery_skips_corrupt_csv) {
  auto ok = aethersense::io::ParseCsvRecord("1,2,1,1,1,0.1,0.2");
  REQUIRE(ok.frame.has_value());
  auto bad = aethersense::io::ParseCsvRecord("1,2");
  REQUIRE(bad.corrupt);
}
