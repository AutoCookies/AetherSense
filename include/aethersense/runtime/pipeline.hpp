#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "aethersense/core/config.hpp"
#include "aethersense/core/errors.hpp"
#include "aethersense/core/types.hpp"
#include "aethersense/runtime/decision_engine.hpp"
#include "aethersense/runtime/metrics.hpp"

namespace aethersense {

struct Decision {
  std::uint64_t timestamp_ns{0};
  float energy_motion{0.0F};
  float energy_breathing{0.0F};
  bool present{false};
};

class Pipeline {
public:
  struct FrameSignals {
    std::uint64_t timestamp_ns{0};
    std::vector<float> amplitude_by_sc;
    std::vector<float> phase_by_sc;
  };

  explicit Pipeline(const Config &config);

  Result<std::optional<Decision>> ProcessFrame(const CsiFrame &frame, RuntimeMetrics &metrics);

private:
  Config config_;
  DecisionEngine decision_engine_;
  std::vector<FrameSignals> window_;
};

} // namespace aethersense
