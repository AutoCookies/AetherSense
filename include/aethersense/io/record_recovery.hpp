#pragma once

#include <optional>
#include <string>

#include "aethersense/core/types.hpp"

namespace aethersense::io {

struct RecoveryResult {
  std::optional<CsiFrame> frame;
  bool corrupt{false};
  std::string error;
};

RecoveryResult ParseCsvRecord(const std::string &line);
RecoveryResult ParseJsonlRecord(const std::string &line);

} // namespace aethersense::io
