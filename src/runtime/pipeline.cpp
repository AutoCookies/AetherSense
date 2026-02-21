#include "aethersense/runtime/pipeline.hpp"

#include <algorithm>
#include <chrono>
#include <complex>
#include <numeric>

#include "aethersense/core/types.hpp"
#include "aethersense/dsp/calibration.hpp"
#include "aethersense/dsp/filters.hpp"
#include "aethersense/dsp/outlier.hpp"
#include "aethersense/dsp/resampler.hpp"
#include "aethersense/dsp/window.hpp"

namespace aethersense {

namespace {

Pipeline::FrameSignals ComputeSignals(const CsiFrame &frame) {
  Pipeline::FrameSignals out;
  out.timestamp_ns = frame.timestamp_ns;
  out.amplitude_by_sc.resize(frame.subcarrier_count, 0.0F);
  out.phase_by_sc.resize(frame.subcarrier_count, 0.0F);

  const std::size_t links = static_cast<std::size_t>(frame.rx_count) * frame.tx_count;
  for (std::uint16_t sc = 0; sc < frame.subcarrier_count; ++sc) {
    float amp_sum = 0.0F;
    std::complex<float> complex_sum(0.0F, 0.0F);
    for (std::uint8_t rx = 0; rx < frame.rx_count; ++rx) {
      for (std::uint8_t tx = 0; tx < frame.tx_count; ++tx) {
        const auto idx = FlatIndex(rx, tx, sc, frame.tx_count, frame.subcarrier_count);
        const auto sample = frame.data[idx];
        amp_sum += std::abs(sample);
        complex_sum += sample;
      }
    }
    out.amplitude_by_sc[sc] = amp_sum / static_cast<float>(links);
    out.phase_by_sc[sc] = std::atan2(complex_sum.imag(), complex_sum.real());
  }
  return out;
}

} // namespace

Pipeline::Pipeline(const Config &config)
    : config_(config), decision_engine_(config.decision.threshold_on, config.decision.threshold_off,
                                        config.decision.hold_frames) {
  window_.reserve(config_.dsp.window_frames);
}

Result<std::optional<Decision>> Pipeline::ProcessFrame(const CsiFrame &frame,
                                                       RuntimeMetrics &metrics) {
  const auto start = std::chrono::steady_clock::now();
  if (frame.data.empty()) {
    return Error{ErrorCode::kInvalidArgument, "frame.data cannot be empty"};
  }

  if (window_.empty()) {
    metrics.shape_change_total = 0;
  } else {
    const auto &last = window_.back();
    if (last.amplitude_by_sc.size() != frame.subcarrier_count) {
      window_.clear();
      ++metrics.shape_change_total;
      return std::optional<Decision>{};
    }
  }

  if (window_.size() == config_.dsp.window_frames) {
    window_.erase(window_.begin());
  }
  window_.push_back(ComputeSignals(frame));
  metrics.window_fill_ratio =
      static_cast<float>(window_.size()) / static_cast<float>(config_.dsp.window_frames);

  if (window_.size() < config_.dsp.window_frames) {
    return std::optional<Decision>{};
  }

  std::vector<std::uint64_t> timestamps;
  timestamps.reserve(window_.size());
  for (const auto &f : window_) {
    timestamps.push_back(f.timestamp_ns);
  }
  const float jitter_ratio = dsp::JitterMetric(timestamps);
  if (jitter_ratio > config_.dsp.resampling.reject_jitter_ratio) {
    ++metrics.windows_rejected_total;
    return std::optional<Decision>{};
  }

  std::vector<std::vector<float>> amp_series(frame.subcarrier_count, std::vector<float>(window_.size()));
  std::vector<std::vector<float>> phase_series(frame.subcarrier_count, std::vector<float>(window_.size()));
  for (std::size_t t = 0; t < window_.size(); ++t) {
    for (std::size_t sc = 0; sc < frame.subcarrier_count; ++sc) {
      amp_series[sc][t] = window_[t].amplitude_by_sc[sc];
      phase_series[sc][t] = window_[t].phase_by_sc[sc];
    }
  }

  dsp::RemoveCommonPhaseError(phase_series, true);
  for (auto &series : phase_series) {
    series = dsp::ResampleToUniformGrid(timestamps, series, config_.dsp.resampling.method);
    dsp::FilterOutliers(series, config_.dsp.outlier.method, config_.dsp.outlier.k,
                        config_.dsp.outlier.window);
    auto uw = dsp::UnwrapPhase(series);
    dsp::RemoveLinearTrend(uw);
    series = std::move(uw);
  }

  const auto selected = dsp::TopKVariance(amp_series, config_.dsp.topk_subcarriers);
  std::vector<float> aggregate(window_.size(), 0.0F);
  for (std::size_t idx : selected) {
    for (std::size_t t = 0; t < aggregate.size(); ++t) {
      aggregate[t] += phase_series[idx][t];
    }
  }
  for (float &v : aggregate) {
    v /= static_cast<float>(selected.size());
  }

  std::vector<float> smoothed = aggregate;
  if (config_.dsp.smoothing.type == "median") {
    smoothed = dsp::MedianSmooth(aggregate, config_.dsp.smoothing.kernel);
  } else {
    smoothed = dsp::EmaSmooth(aggregate, config_.dsp.smoothing.alpha);
  }

  std::vector<std::uint64_t> dtns;
  for (std::size_t i = 1; i < timestamps.size(); ++i)
    dtns.push_back(timestamps[i] - timestamps[i - 1]);
  std::sort(dtns.begin(), dtns.end());
  const float sample_rate = 1e9F / static_cast<float>(dtns[dtns.size() / 2]);

  dsp::ApplyWindow(smoothed, dsp::ParseWindowType(config_.dsp.fft.window));
  const std::size_t fft_len =
      config_.dsp.fft.zero_pad_pow2 ? dsp::NextPow2(smoothed.size()) : smoothed.size();
  const auto spectrum = dsp::MagnitudeSpectrum(smoothed, config_.dsp.fft.zero_pad_pow2);

  const float energy_motion =
      dsp::BandEnergy(spectrum, sample_rate, config_.dsp.bands.motion.low_hz,
                      config_.dsp.bands.motion.high_hz, fft_len);
  float energy_breathing = 0.0F;
  if (config_.dsp.bands.breathing.enabled) {
    energy_breathing = dsp::BandEnergy(spectrum, sample_rate, config_.dsp.bands.breathing.low_hz,
                                       config_.dsp.bands.breathing.high_hz, fft_len);
  }

  const bool present = decision_engine_.Update(energy_motion);

  const auto end = std::chrono::steady_clock::now();
  metrics.AddProcessingTimeUs(
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
  ++metrics.frames_processed_total;

  return std::optional<Decision>{
      Decision{frame.timestamp_ns, energy_motion, energy_breathing, present}};
}

} // namespace aethersense
