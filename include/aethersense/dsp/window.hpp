#pragma once

#include <cmath>
#include <string>
#include <vector>

namespace aethersense::dsp {

enum class WindowType { kHann, kHamming };

inline WindowType ParseWindowType(const std::string &name) {
  return name == "hamming" ? WindowType::kHamming : WindowType::kHann;
}

inline std::vector<float> BuildWindow(WindowType type, std::size_t n) {
  std::vector<float> out(n, 1.0F);
  if (n <= 1) {
    return out;
  }
  const float denom = static_cast<float>(n - 1);
  for (std::size_t i = 0; i < n; ++i) {
    constexpr float kPi = 3.14159265358979323846F;
    const float phase = 2.0F * kPi * static_cast<float>(i) / denom;
    out[i] = type == WindowType::kHamming ? 0.54F - 0.46F * std::cos(phase)
                                          : 0.5F * (1.0F - std::cos(phase));
  }
  return out;
}

inline void ApplyWindow(std::vector<float> &data, WindowType type) {
  const auto win = BuildWindow(type, data.size());
  for (std::size_t i = 0; i < data.size(); ++i) {
    data[i] *= win[i];
  }
}

} // namespace aethersense::dsp
