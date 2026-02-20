#pragma once

#include <string>
#include <vector>

#include "aethersense/core/config.hpp"
#include "aethersense/core/errors.hpp"
#include "aethersense/core/types.hpp"

namespace aethersense {

struct Decision {
  std::uint64_t timestamp_ns{0};
  float energy{0.0F};
  bool present{false};
};

class Pipeline {
public:
  explicit Pipeline(float presence_threshold, std::vector<std::string> enabled_stages);
  Result<Decision> Process(const CsiFrame &frame) const;

private:
  [[nodiscard]] bool IsEnabled(const std::string &stage) const;

  float presence_threshold_;
  std::vector<std::string> enabled_stages_;
};

} // namespace aethersense
