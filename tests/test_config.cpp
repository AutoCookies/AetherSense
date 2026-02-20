#include "test_harness.hpp"

#include "aethersense/core/config.hpp"

TEST_CASE(Config_v2_validates_nominal_values) {
  aethersense::Config cfg;
  cfg.config_version = 2;
  cfg.io.format = "csv";
  cfg.io.path = "../testdata/csi_small.csv";
  cfg.dsp.window_frames = 16;
  cfg.dsp.topk_subcarriers = 2;
  cfg.decision.threshold_on = 0.2F;
  cfg.decision.threshold_off = 0.1F;

  const auto result = aethersense::ValidateConfig(cfg, false, 4);
  REQUIRE(result.ok());
}

TEST_CASE(Config_v2_rejects_invalid_values) {
  aethersense::Config cfg;
  cfg.config_version = 1;
  auto result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(!result.ok());

  cfg.config_version = 2;
  cfg.decision.threshold_on = 0.1F;
  cfg.decision.threshold_off = 0.2F;
  result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(!result.ok());

  cfg.decision.threshold_on = 0.2F;
  cfg.decision.threshold_off = 0.1F;
  cfg.dsp.window_frames = 8;
  result = aethersense::ValidateConfig(cfg, false);
  REQUIRE(!result.ok());
}

TEST_CASE(Load_config_v2_from_JSON) {
  const auto result = aethersense::LoadConfigFromJsonFile("../testdata/sample_config.json");
  REQUIRE(result.ok());
  REQUIRE(result.value().config_version == 2);
  REQUIRE(result.value().dsp.window_frames == 16);
}
