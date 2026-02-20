#include "test_harness.hpp"

#include "aethersense/io/csi_reader.hpp"
#include "aethersense/runtime/pipeline.hpp"

TEST_CASE(Pipeline_parse_to_decision_smoke) {
  auto reader = aethersense::CreateReader("csv", "../testdata/csi_small.csv");
  REQUIRE(reader.ok());

  aethersense::Pipeline pipeline(2.0F, {"normalize", "feature", "presence_decision"});
  const auto frame = reader.value()->next();
  REQUIRE(frame.ok());
  REQUIRE(frame.value().has_value());

  const auto decision = pipeline.Process(*frame.value());
  REQUIRE(decision.ok());
  REQUIRE(decision.value().timestamp_ns == 1000);
  REQUIRE(decision.value().present);
}

TEST_CASE(Pipeline_golden_energy_test) {
  aethersense::CsiFrame frame;
  frame.timestamp_ns = 1;
  frame.center_freq_hz = 1;
  frame.rx_count = 1;
  frame.tx_count = 1;
  frame.subcarrier_count = 2;
  frame.data = {std::complex<float>(3.0F, 4.0F), std::complex<float>(0.0F, 0.0F)};

  aethersense::Pipeline pipeline(2.0F, {"feature", "presence_decision"});
  const auto decision = pipeline.Process(frame);
  REQUIRE(decision.ok());
  REQUIRE_NEAR(decision.value().energy, 2.5F, 1e-6F);
  REQUIRE(decision.value().present);
}
