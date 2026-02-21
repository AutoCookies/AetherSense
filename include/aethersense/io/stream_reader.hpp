#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <memory>
#include <string>

#include "aethersense/core/config.hpp"
#include "aethersense/core/errors.hpp"

namespace aethersense::io {

struct StreamStats {
  std::size_t records_total{0};
  std::size_t records_corrupt_total{0};
  std::size_t records_partial_total{0};
  std::size_t rotations_detected_total{0};
  std::size_t checkpoint_writes_total{0};
  std::size_t checkpoint_resume_total{0};
  std::size_t consecutive_errors_current{0};
};

struct StreamRecord {
  std::string line;
  bool eof{false};
};

class IStreamReader {
public:
  virtual ~IStreamReader() = default;
  virtual Result<bool> open(const std::string &path) = 0;
  virtual Result<StreamRecord> read_next() = 0;
  virtual StreamStats stats() const = 0;
  virtual std::uint64_t last_timestamp_ns() const = 0;
};

Result<std::unique_ptr<IStreamReader>> CreateStreamReader(const Config::Io &cfg);

} // namespace aethersense::io
