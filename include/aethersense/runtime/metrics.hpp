#pragma once

#include <cstddef>

namespace aethersense {

struct StreamMetrics {
  std::size_t frames{0};
  double energy_sum{0.0};
  std::size_t present_count{0};
};

} // namespace aethersense
