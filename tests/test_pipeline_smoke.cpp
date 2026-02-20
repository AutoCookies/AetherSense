#include "test_harness.hpp"

#include "aethersense/core/config.hpp"
#include "aethersense/io/csi_reader.hpp"
#include "aethersense/runtime/metrics.hpp"
#include "aethersense/runtime/pipeline.hpp"

TEST_CASE(Pipeline_end_to_end_produces_stable_decisions) {
  auto reader = aethersense::CreateReader("csv", "../testdata/csi_small.csv");
  REQUIRE(reader.ok());

  aethersense::Config cfg;
  cfg.dsp.window_frames = 16;
  cfg.dsp.topk_subcarriers = 1;
  cfg.decision.threshold_on = 1e-8F;
  cfg.decision.threshold_off = 5e-9F;
  cfg.decision.hold_frames = 2;
  cfg.dsp.smoothing.type = "ema";
  cfg.dsp.smoothing.alpha = 0.3F;
  cfg.dsp.fft.window = "hann";
  cfg.runtime.max_jitter_ratio = 0.4F;

  aethersense::Pipeline pipeline(cfg);
  aethersense::RuntimeMetrics metrics;

  int decision_count = 0;
  int present_count = 0;
  float last_energy = 0.0F;
  while (true) {
    auto frame = reader.value()->next();
    REQUIRE(frame.ok());
    if (!frame.value().has_value()) {
      break;
    }
    auto decision = pipeline.ProcessFrame(*frame.value(), metrics);
    REQUIRE(decision.ok());
    if (decision.value().has_value()) {
      ++decision_count;
      if (decision.value()->present) {
        ++present_count;
      }
      last_energy = decision.value()->energy_motion;
    }
  }
  REQUIRE(decision_count > 0);
  REQUIRE(last_energy >= 0.0F);
  REQUIRE(present_count >= 0);
}

TEST_CASE(Pipeline_rejects_jittery_windows) {
  aethersense::Config cfg;
  cfg.dsp.window_frames = 16;
  cfg.dsp.topk_subcarriers = 1;
  cfg.runtime.max_jitter_ratio = 0.01F;
  cfg.decision.threshold_on = 0.01F;
  cfg.decision.threshold_off = 0.005F;
  aethersense::Pipeline pipeline(cfg);
  aethersense::RuntimeMetrics metrics;

  for (int i = 0; i < 16; ++i) {
    aethersense::CsiFrame frame;
    frame.timestamp_ns =
        1000000000ULL + static_cast<std::uint64_t>(i) * (i % 2 == 0 ? 100000000ULL : 150000000ULL);
    frame.center_freq_hz = 5800000000ULL;
    frame.rx_count = 1;
    frame.tx_count = 1;
    frame.subcarrier_count = 1;
    frame.data = {std::complex<float>(1.0F, 0.1F * static_cast<float>(i))};
    auto result = pipeline.ProcessFrame(frame, metrics);
    REQUIRE(result.ok());
  }
  REQUIRE(metrics.windows_rejected_total > 0);
}
