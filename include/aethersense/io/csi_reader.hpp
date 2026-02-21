#pragma once

#include <memory>
#include <optional>
#include <string>

#include "aethersense/core/config.hpp"
#include "aethersense/core/errors.hpp"
#include "aethersense/core/types.hpp"
#include "aethersense/io/stream_reader.hpp"

namespace aethersense {

class ICsiReader {
public:
  virtual ~ICsiReader() = default;
  virtual Result<std::optional<CsiFrame>> next() = 0;
  virtual io::StreamStats stream_stats() const = 0;
};

Result<std::unique_ptr<ICsiReader>> CreateReader(const Config::Io &io_cfg, const std::string &path);

} // namespace aethersense
