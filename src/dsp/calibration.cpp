#include "aethersense/dsp/calibration.hpp"

#include <algorithm>
#include <numeric>

namespace aethersense::dsp {

void RemoveCommonPhaseError(std::vector<std::vector<float>> &phase_by_sc, bool robust_median) {
  if (phase_by_sc.empty() || phase_by_sc[0].empty())
    return;
  const std::size_t t_count = phase_by_sc[0].size();
  for (std::size_t t = 0; t < t_count; ++t) {
    std::vector<float> vals;
    vals.reserve(phase_by_sc.size());
    for (const auto &series : phase_by_sc) {
      vals.push_back(series[t]);
    }
    float cpe = 0.0F;
    if (robust_median) {
      std::sort(vals.begin(), vals.end());
      cpe = vals[vals.size() / 2];
    } else {
      cpe = std::accumulate(vals.begin(), vals.end(), 0.0F) / static_cast<float>(vals.size());
    }
    for (auto &series : phase_by_sc) {
      series[t] -= cpe;
    }
  }
}

void RemoveLinearTrend(std::vector<float> &series) {
  if (series.size() < 2)
    return;
  const float first = series.front();
  const float last = series.back();
  for (std::size_t i = 0; i < series.size(); ++i) {
    const float trend = first + (last - first) * (static_cast<float>(i) / (series.size() - 1));
    series[i] -= trend;
  }
}

} // namespace aethersense::dsp
