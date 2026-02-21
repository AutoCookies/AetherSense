#include "test_harness.hpp"

#include "aethersense/core/config.hpp"
#include "aethersense/runtime/metrics.hpp"
#include "aethersense/runtime/pipeline.hpp"

TEST_CASE(Short_soak_pipeline_no_crash) {
  aethersense::Config cfg;
  cfg.config_version = 3;
  cfg.dsp.window_frames = 16;
  cfg.dsp.topk_subcarriers = 1;
  cfg.dsp.resampling.method = "linear";
  cfg.dsp.outlier.method = "mad";
  aethersense::Pipeline p(cfg);
  aethersense::RuntimeMetrics m;
  for (int i = 0; i < 500; ++i) {
    aethersense::CsiFrame f;
    f.timestamp_ns = 1000000000ULL + static_cast<std::uint64_t>(i) * 50000000ULL;
    f.center_freq_hz = 5800000000ULL;
    f.rx_count = 1; f.tx_count = 1; f.subcarrier_count = 1;
    f.data.push_back({1.0F, 0.0F});
    auto r = p.ProcessFrame(f, m);
    REQUIRE(r.ok());
  }
}
