#pragma once

#include <memory>
#include <optional>
#include <string>

#include "aethersense/core/errors.hpp"
#include "aethersense/core/types.hpp"

namespace aethersense {

class ICsiReader {
public:
  virtual ~ICsiReader() = default;
  virtual Result<std::optional<CsiFrame>> next() = 0;
};

Result<std::unique_ptr<ICsiReader>> CreateReader(const std::string &format,
                                                 const std::string &path);

} // namespace aethersense
