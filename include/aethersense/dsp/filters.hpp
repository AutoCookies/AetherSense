#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

namespace aethersense::dsp {

inline float Median(std::vector<float> v) {
  if (v.empty()) {
    return 0.0F;
  }
  std::sort(v.begin(), v.end());
  const std::size_t m = v.size() / 2;
  return (v.size() % 2 == 0) ? 0.5F * (v[m - 1] + v[m]) : v[m];
}

inline float MedianDeltaSeconds(const std::vector<std::uint64_t> &timestamps_ns) {
  if (timestamps_ns.size() < 2) {
    return 0.0F;
  }
  std::vector<float> deltas;
  deltas.reserve(timestamps_ns.size() - 1);
  for (std::size_t i = 1; i < timestamps_ns.size(); ++i) {
    deltas.push_back(static_cast<float>(timestamps_ns[i] - timestamps_ns[i - 1]) / 1e9F);
  }
  return Median(deltas);
}

inline float JitterRatio(const std::vector<std::uint64_t> &timestamps_ns, float median_dt) {
  if (timestamps_ns.size() < 2 || median_dt <= 0.0F) {
    return 0.0F;
  }
  float max_ratio = 0.0F;
  for (std::size_t i = 1; i < timestamps_ns.size(); ++i) {
    const float dt = static_cast<float>(timestamps_ns[i] - timestamps_ns[i - 1]) / 1e9F;
    const float ratio = std::fabs(dt - median_dt) / median_dt;
    max_ratio = std::max(max_ratio, ratio);
  }
  return max_ratio;
}

inline std::vector<float> UnwrapPhase(const std::vector<float> &phase) {
  if (phase.empty()) {
    return {};
  }
  constexpr float kPi = 3.14159265358979323846F;
  constexpr float kTwoPi = 2.0F * kPi;
  std::vector<float> out(phase.size());
  out[0] = phase[0];
  for (std::size_t i = 1; i < phase.size(); ++i) {
    float delta = phase[i] - phase[i - 1];
    if (delta > kPi) {
      delta -= kTwoPi;
    } else if (delta < -kPi) {
      delta += kTwoPi;
    }
    out[i] = out[i - 1] + delta;
  }
  return out;
}

inline std::vector<float> Detrend(const std::vector<float> &x) {
  const std::size_t n = x.size();
  if (n < 2) {
    return x;
  }
  double sum_t = 0.0;
  double sum_y = 0.0;
  double sum_tt = 0.0;
  double sum_ty = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double t = static_cast<double>(i);
    const double y = static_cast<double>(x[i]);
    sum_t += t;
    sum_y += y;
    sum_tt += t * t;
    sum_ty += t * y;
  }
  const double denom = n * sum_tt - sum_t * sum_t;
  const double slope = denom == 0.0 ? 0.0 : (n * sum_ty - sum_t * sum_y) / denom;
  const double intercept = (sum_y - slope * sum_t) / static_cast<double>(n);

  std::vector<float> out(n);
  for (std::size_t i = 0; i < n; ++i) {
    out[i] = static_cast<float>(x[i] - (slope * static_cast<double>(i) + intercept));
  }
  return out;
}

inline std::vector<float> EmaSmooth(const std::vector<float> &x, float alpha) {
  if (x.empty()) {
    return {};
  }
  std::vector<float> out(x.size());
  out[0] = x[0];
  for (std::size_t i = 1; i < x.size(); ++i) {
    out[i] = alpha * x[i] + (1.0F - alpha) * out[i - 1];
  }
  return out;
}

inline std::vector<float> MedianSmooth(const std::vector<float> &x, int kernel) {
  if (x.empty() || kernel <= 1) {
    return x;
  }
  const int radius = kernel / 2;
  std::vector<float> out(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) {
    std::vector<float> local;
    for (int k = -radius; k <= radius; ++k) {
      const int idx = static_cast<int>(i) + k;
      if (idx >= 0 && idx < static_cast<int>(x.size())) {
        local.push_back(x[static_cast<std::size_t>(idx)]);
      }
    }
    out[i] = Median(local);
  }
  return out;
}

inline std::size_t NextPow2(std::size_t n) {
  std::size_t p = 1;
  while (p < n) {
    p <<= 1U;
  }
  return p;
}

inline void FftInPlace(std::vector<std::complex<float>> &a) {
  const std::size_t n = a.size();
  for (std::size_t i = 1, j = 0; i < n; ++i) {
    std::size_t bit = n >> 1U;
    while (j & bit) {
      j ^= bit;
      bit >>= 1U;
    }
    j ^= bit;
    if (i < j) {
      std::swap(a[i], a[j]);
    }
  }
  constexpr float kPi = 3.14159265358979323846F;
  for (std::size_t len = 2; len <= n; len <<= 1U) {
    const float angle = -2.0F * kPi / static_cast<float>(len);
    const std::complex<float> wlen(std::cos(angle), std::sin(angle));
    for (std::size_t i = 0; i < n; i += len) {
      std::complex<float> w(1.0F, 0.0F);
      for (std::size_t j = 0; j < len / 2; ++j) {
        const std::complex<float> u = a[i + j];
        const std::complex<float> v = a[i + j + len / 2] * w;
        a[i + j] = u + v;
        a[i + j + len / 2] = u - v;
        w *= wlen;
      }
    }
  }
}

inline std::vector<float> MagnitudeSpectrum(const std::vector<float> &signal, bool zero_pad_pow2) {
  std::size_t n = signal.size();
  if (zero_pad_pow2) {
    n = NextPow2(n);
  }
  std::vector<std::complex<float>> data(n, {0.0F, 0.0F});
  for (std::size_t i = 0; i < signal.size(); ++i) {
    data[i] = {signal[i], 0.0F};
  }
  FftInPlace(data);
  std::vector<float> mag(n / 2, 0.0F);
  for (std::size_t i = 0; i < mag.size(); ++i) {
    mag[i] = std::abs(data[i]);
  }
  return mag;
}

inline float BandEnergy(const std::vector<float> &spectrum, float sample_rate_hz, float low_hz,
                        float high_hz, std::size_t fft_len) {
  if (spectrum.empty() || sample_rate_hz <= 0.0F) {
    return 0.0F;
  }
  float energy = 0.0F;
  for (std::size_t i = 0; i < spectrum.size(); ++i) {
    const float freq = sample_rate_hz * static_cast<float>(i) / static_cast<float>(fft_len);
    if (freq >= low_hz && freq <= high_hz) {
      energy += spectrum[i] * spectrum[i];
    }
  }
  return energy;
}

inline std::vector<std::size_t> TopKVariance(const std::vector<std::vector<float>> &series_by_sc,
                                             std::size_t k) {
  std::vector<std::pair<float, std::size_t>> variance_index;
  variance_index.reserve(series_by_sc.size());
  for (std::size_t sc = 0; sc < series_by_sc.size(); ++sc) {
    const auto &s = series_by_sc[sc];
    if (s.empty()) {
      variance_index.push_back({0.0F, sc});
      continue;
    }
    const float mean = std::accumulate(s.begin(), s.end(), 0.0F) / static_cast<float>(s.size());
    float var = 0.0F;
    for (float v : s) {
      const float d = v - mean;
      var += d * d;
    }
    var /= static_cast<float>(s.size());
    variance_index.push_back({var, sc});
  }
  std::sort(variance_index.begin(), variance_index.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });
  k = std::min(k, variance_index.size());
  std::vector<std::size_t> out;
  out.reserve(k);
  for (std::size_t i = 0; i < k; ++i) {
    out.push_back(variance_index[i].second);
  }
  return out;
}

} // namespace aethersense::dsp
