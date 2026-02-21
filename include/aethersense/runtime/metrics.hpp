#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <vector>

namespace aethersense {

struct RuntimeMetrics {
  std::size_t frames_read_total{0};
  std::size_t frames_processed_total{0};
  std::size_t frames_dropped_total{0};
  std::size_t windows_rejected_total{0};
  std::size_t shape_change_total{0};
  std::size_t ring_buffer_depth{0};
  float window_fill_ratio{0.0F};

  std::deque<double> processing_time_us;
  std::size_t latency_window{64};

  void AddProcessingTimeUs(double value) {
    processing_time_us.push_back(value);
    while (processing_time_us.size() > latency_window) {
      processing_time_us.pop_front();
    }
  }

  [[nodiscard]] double Percentile(double p) const {
    if (processing_time_us.empty()) {
      return 0.0;
    }
    std::vector<double> sorted(processing_time_us.begin(), processing_time_us.end());
    std::sort(sorted.begin(), sorted.end());
    const std::size_t idx =
        static_cast<std::size_t>((p / 100.0) * static_cast<double>(sorted.size() - 1));
    return sorted[idx];
  }
};

} // namespace aethersense
