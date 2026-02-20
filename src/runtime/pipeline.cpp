#include "aethersense/runtime/pipeline.hpp"

#include <algorithm>
#include <cmath>

namespace aethersense {

Pipeline::Pipeline(float presence_threshold, std::vector<std::string> enabled_stages)
    : presence_threshold_(presence_threshold), enabled_stages_(std::move(enabled_stages)) {}

bool Pipeline::IsEnabled(const std::string &stage) const {
  return std::find(enabled_stages_.begin(), enabled_stages_.end(), stage) != enabled_stages_.end();
}

Result<Decision> Pipeline::Process(const CsiFrame &frame) const {
  if (frame.data.empty()) {
    return Error{ErrorCode::kInvalidArgument, "frame.data cannot be empty"};
  }

  (void)IsEnabled("normalize");

  float energy = 0.0F;
  if (IsEnabled("feature")) {
    for (const auto &sample : frame.data) {
      energy += std::abs(sample);
    }
    energy /= static_cast<float>(frame.data.size());
  }

  bool present = false;
  if (IsEnabled("presence_decision")) {
    present = energy >= presence_threshold_;
  }

  return Decision{frame.timestamp_ns, energy, present};
}

} // namespace aethersense
