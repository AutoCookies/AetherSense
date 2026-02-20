#include "aethersense/core/config.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace aethersense {
namespace {

bool IsFormatSupported(const std::string &format) { return format == "csv" || format == "jsonl"; }

Result<std::string> ExtractString(const std::string &text, const std::string &key) {
  const std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
  std::smatch m;
  if (!std::regex_search(text, m, re)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return m[1].str();
}

Result<std::size_t> ExtractSize(const std::string &text, const std::string &key) {
  const std::regex re("\"" + key + "\"\\s*:\\s*([0-9]+)");
  std::smatch m;
  if (!std::regex_search(text, m, re)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return static_cast<std::size_t>(std::stoull(m[1].str()));
}

Result<float> ExtractFloat(const std::string &text, const std::string &key) {
  const std::regex re("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
  std::smatch m;
  if (!std::regex_search(text, m, re)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return std::stof(m[1].str());
}

Result<std::vector<std::string>> ExtractStringArray(const std::string &text,
                                                    const std::string &key) {
  const std::regex re("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
  std::smatch m;
  if (!std::regex_search(text, m, re)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  std::vector<std::string> out;
  const std::regex item_re("\"([^\"]+)\"");
  auto begin = std::sregex_iterator(m[1].first, m[1].second, item_re);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    out.push_back((*it)[1].str());
  }
  return out;
}

} // namespace

Result<bool> ValidateConfig(const Config &cfg, bool require_existing_path) {
  if (!IsFormatSupported(cfg.io.format)) {
    return Error{ErrorCode::kUnsupportedFormat, "io.format must be csv or jsonl"};
  }
  if (cfg.runtime.ring_buffer_capacity_frames < 8) {
    return Error{ErrorCode::kInvalidConfig, "runtime.ring_buffer_capacity_frames must be >= 8"};
  }
  if (cfg.runtime.max_batch_frames > cfg.runtime.ring_buffer_capacity_frames) {
    return Error{ErrorCode::kInvalidConfig, "runtime.max_batch_frames must be <= capacity"};
  }
  if (cfg.pipeline.presence_threshold < 0.0F || cfg.pipeline.presence_threshold > 1e6F) {
    return Error{ErrorCode::kInvalidConfig, "pipeline.presence_threshold out of sanity range"};
  }
  if (!(cfg.runtime.clock == "monotonic" || cfg.runtime.clock == "from_input")) {
    return Error{ErrorCode::kInvalidConfig, "runtime.clock must be monotonic or from_input"};
  }
  if (require_existing_path && !std::filesystem::exists(cfg.io.path)) {
    return Error{ErrorCode::kIoError, "io.path does not exist: " + cfg.io.path};
  }
  return true;
}

Result<Config> LoadConfigFromJsonFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return Error{ErrorCode::kIoError, "Failed to open config file: " + path};
  }

  std::stringstream buffer;
  buffer << in.rdbuf();
  const std::string text = buffer.str();

  Config cfg;
  auto format = ExtractString(text, "format");
  if (!format.ok())
    return format.error();
  cfg.io.format = format.value();

  auto io_path = ExtractString(text, "path");
  if (!io_path.ok())
    return io_path.error();
  cfg.io.path = io_path.value();

  auto cap = ExtractSize(text, "ring_buffer_capacity_frames");
  if (!cap.ok())
    return cap.error();
  cfg.runtime.ring_buffer_capacity_frames = cap.value();

  auto batch = ExtractSize(text, "max_batch_frames");
  if (!batch.ok())
    return batch.error();
  cfg.runtime.max_batch_frames = batch.value();

  auto clock = ExtractString(text, "clock");
  if (!clock.ok())
    return clock.error();
  cfg.runtime.clock = clock.value();

  auto level = ExtractString(text, "level");
  if (!level.ok())
    return level.error();
  cfg.logging.level = level.value();

  auto stages = ExtractStringArray(text, "enabled_stages");
  if (!stages.ok())
    return stages.error();
  cfg.pipeline.enabled_stages = stages.value();

  auto threshold = ExtractFloat(text, "presence_threshold");
  if (!threshold.ok())
    return threshold.error();
  cfg.pipeline.presence_threshold = threshold.value();

  auto valid = ValidateConfig(cfg, false);
  if (!valid.ok()) {
    return valid.error();
  }
  return cfg;
}

} // namespace aethersense
