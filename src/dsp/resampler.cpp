#include "aethersense/dsp/resampler.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>

namespace aethersense::dsp {

float JitterMetric(const std::vector<std::uint64_t> &timestamps_ns) {
  if (timestamps_ns.size() < 3)
    return 0.0F;
  std::vector<float> dt;
  dt.reserve(timestamps_ns.size() - 1);
  for (std::size_t i = 1; i < timestamps_ns.size(); ++i) {
    dt.push_back(static_cast<float>(timestamps_ns[i] - timestamps_ns[i - 1]) / 1e9F);
  }
  std::vector<float> sorted = dt;
  std::sort(sorted.begin(), sorted.end());
  const float med = sorted[sorted.size() / 2];
  if (med <= 0.0F)
    return 1.0F;
  const float mean = std::accumulate(dt.begin(), dt.end(), 0.0F) / static_cast<float>(dt.size());
  float var = 0.0F;
  for (float v : dt) {
    const float d = v - mean;
    var += d * d;
  }
  var /= static_cast<float>(dt.size());
  return std::sqrt(var) / med;
}

std::vector<float> ResampleToUniformGrid(const std::vector<std::uint64_t> &timestamps_ns,
                                         const std::vector<float> &samples,
                                         const std::string &method) {
  if (timestamps_ns.size() != samples.size() || samples.size() < 2)
    return samples;

  std::vector<std::uint64_t> dtns;
  for (std::size_t i = 1; i < timestamps_ns.size(); ++i) {
    dtns.push_back(timestamps_ns[i] - timestamps_ns[i - 1]);
  }
  std::sort(dtns.begin(), dtns.end());
  const std::uint64_t step = dtns[dtns.size() / 2];

  std::vector<float> out(samples.size(), 0.0F);
  std::size_t src = 0;
  for (std::size_t i = 0; i < out.size(); ++i) {
    const auto t = timestamps_ns.front() + static_cast<std::uint64_t>(i) * step;
    while (src + 1 < timestamps_ns.size() && timestamps_ns[src + 1] < t) {
      ++src;
    }
    if (src + 1 >= timestamps_ns.size()) {
      out[i] = samples.back();
      continue;
    }
    if (method == "nearest") {
      const auto dl = t - timestamps_ns[src];
      const auto dr = timestamps_ns[src + 1] - t;
      out[i] = (dl <= dr) ? samples[src] : samples[src + 1];
      continue;
    }
    const float t0 = static_cast<float>(timestamps_ns[src]);
    const float t1 = static_cast<float>(timestamps_ns[src + 1]);
    const float a = (static_cast<float>(t) - t0) / (t1 - t0 + 1e-9F);
    out[i] = samples[src] + a * (samples[src + 1] - samples[src]);
  }
  return out;
}

} // namespace aethersense::dsp
