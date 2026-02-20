#include "test_harness.hpp"

#include "aethersense/core/config.hpp"

TEST_CASE(Config_validates_nominal_values) {
  aethersense::Config cfg;
  cfg.io.format = "csv";
  cfg.io.path = "../testdata/csi_small.csv";
  cfg.runtime.ring_buffer_capacity_frames = 16;
  cfg.runtime.max_batch_frames = 8;
  cfg.runtime.clock = "from_input";
  cfg.pipeline.presence_threshold = 2.0F;

  const auto result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(result.ok());
}

TEST_CASE(Config_rejects_invalid_values) {
  aethersense::Config cfg;
  cfg.io.format = "bad";
  auto result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(!result.ok());

  cfg.io.format = "csv";
  cfg.runtime.ring_buffer_capacity_frames = 4;
  result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(!result.ok());

  cfg.runtime.ring_buffer_capacity_frames = 16;
  cfg.runtime.max_batch_frames = 17;
  result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(!result.ok());
}

TEST_CASE(Load_config_from_JSON) {
  const auto result = aethersense::LoadConfigFromJsonFile("../testdata/sample_config.json");
  REQUIRE(result.ok());
  REQUIRE(result.value().io.format == "csv");
}
