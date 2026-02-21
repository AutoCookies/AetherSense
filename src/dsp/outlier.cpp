#include "aethersense/dsp/outlier.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace aethersense::dsp {

void FilterOutliers(std::vector<float> &series, const std::string &method, float k, int window) {
  if (series.empty() || window < 3)
    return;
  const int half = window / 2;
  for (int i = 0; i < static_cast<int>(series.size()); ++i) {
    const int s = std::max(0, i - half);
    const int e = std::min(static_cast<int>(series.size()), i + half + 1);
    std::vector<float> local(series.begin() + s, series.begin() + e);
    std::sort(local.begin(), local.end());
    const float med = local[local.size() / 2];
    std::vector<float> dev;
    dev.reserve(local.size());
    for (float v : local)
      dev.push_back(std::fabs(v - med));
    std::sort(dev.begin(), dev.end());
    const float mad = std::max(1e-6F, dev[dev.size() / 2]);
    const float z = std::fabs(series[i] - med) / mad;
    if (z > k) {
      if (method == "hampel") {
        series[i] = med;
      } else {
        const float left = (i > 0) ? series[i - 1] : med;
        const float right = (i + 1 < static_cast<int>(series.size())) ? series[i + 1] : med;
        series[i] = 0.5F * (left + right);
      }
    }
  }
}

} // namespace aethersense::dsp
