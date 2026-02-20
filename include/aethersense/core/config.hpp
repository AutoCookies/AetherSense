#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "aethersense/core/errors.hpp"

namespace aethersense {

struct Config {
  struct Io {
    std::string format{"csv"};
    std::string path{};
  } io;

  struct Runtime {
    std::size_t ring_buffer_capacity_frames{64};
    std::size_t max_batch_frames{16};
    std::string clock{"from_input"};
  } runtime;

  struct Logging {
    std::string level{"info"};
  } logging;

  struct Pipeline {
    std::vector<std::string> enabled_stages{"normalize", "feature", "presence_decision"};
    float presence_threshold{1.0F};
  } pipeline;
};

Result<Config> LoadConfigFromJsonFile(const std::string &path);
Result<bool> ValidateConfig(const Config &cfg, bool require_existing_path);

} // namespace aethersense
