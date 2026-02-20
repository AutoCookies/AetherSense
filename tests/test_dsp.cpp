#include <cmath>

#include "test_harness.hpp"

#include "aethersense/dsp/filters.hpp"
#include "aethersense/dsp/window.hpp"

TEST_CASE(Window_function_correctness) {
  auto hann = aethersense::dsp::BuildWindow(aethersense::dsp::WindowType::kHann, 5);
  REQUIRE_NEAR(hann.front(), 0.0F, 1e-6F);
  REQUIRE_NEAR(hann.back(), 0.0F, 1e-6F);

  auto hamming = aethersense::dsp::BuildWindow(aethersense::dsp::WindowType::kHamming, 5);
  REQUIRE(hamming.front() > 0.0F);
}

TEST_CASE(FFT_band_energy_peaks_at_sine_frequency) {
  const float fs = 16.0F;
  std::vector<float> x(32, 0.0F);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = std::sin(2.0F * 3.14159265F * 2.0F * static_cast<float>(i) / fs);
  }
  aethersense::dsp::ApplyWindow(x, aethersense::dsp::WindowType::kHann);
  const auto spec = aethersense::dsp::MagnitudeSpectrum(x, true);
  const auto fft_len = aethersense::dsp::NextPow2(x.size());
  const float e_in = aethersense::dsp::BandEnergy(spec, fs, 1.5F, 2.5F, fft_len);
  const float e_out = aethersense::dsp::BandEnergy(spec, fs, 4.0F, 5.0F, fft_len);
  REQUIRE(e_in > e_out);
}

TEST_CASE(Phase_unwrap_and_detrend_reduce_ramp) {
  std::vector<float> phase;
  for (int i = 0; i < 32; ++i) {
    phase.push_back(std::fmod(0.4F * static_cast<float>(i), 2.0F * 3.14159265F));
  }
  const auto unwrapped = aethersense::dsp::UnwrapPhase(phase);
  const auto detrended = aethersense::dsp::Detrend(unwrapped);
  const float mean = std::accumulate(detrended.begin(), detrended.end(), 0.0F) /
                     static_cast<float>(detrended.size());
  REQUIRE(std::fabs(mean) < 1e-3F);
}

TEST_CASE(Subcarrier_selection_topk_variance) {
  std::vector<std::vector<float>> per_sc = {{1, 1, 1, 1}, {1, 3, 1, 3}, {1, 6, 1, 6}, {1, 2, 1, 2}};
  auto top = aethersense::dsp::TopKVariance(per_sc, 2);
  REQUIRE(top.size() == 2);
  REQUIRE(top[0] == 2);
}
