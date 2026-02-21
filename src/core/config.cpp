#include "aethersense/core/config.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace aethersense {
namespace {

template <typename T> bool ExtractOptional(const std::string &text, const std::string &pat, T &out);

template <> bool ExtractOptional<std::string>(const std::string &text, const std::string &key,
                                              std::string &out) {
  std::regex rg("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
  std::smatch m;
  if (!std::regex_search(text, m, rg))
    return false;
  out = m[1].str();
  return true;
}
template <> bool ExtractOptional<int>(const std::string &text, const std::string &key, int &out) {
  std::regex rg("\\\"" + key + "\\\"\\s*:\\s*([-+]?[0-9]+)");
  std::smatch m;
  if (!std::regex_search(text, m, rg))
    return false;
  out = std::stoi(m[1].str());
  return true;
}
template <> bool ExtractOptional<float>(const std::string &text, const std::string &key,
                                        float &out) {
  std::regex rg("\\\"" + key + "\\\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
  std::smatch m;
  if (!std::regex_search(text, m, rg))
    return false;
  out = std::stof(m[1].str());
  return true;
}
template <> bool ExtractOptional<bool>(const std::string &text, const std::string &key,
                                       bool &out) {
  std::regex rg("\\\"" + key + "\\\"\\s*:\\s*(true|false)");
  std::smatch m;
  if (!std::regex_search(text, m, rg))
    return false;
  out = m[1].str() == "true";
  return true;
}

} // namespace

Result<bool> ValidateConfig(const Config &cfg, bool require_existing_path,
                            std::size_t detected_subcarrier_count) {
  if (cfg.config_version != 3) {
    return Error{ErrorCode::kInvalidConfig, "config_version must be 3"};
  }
  if (cfg.io.mode != "file" && cfg.io.mode != "tail")
    return Error{ErrorCode::kInvalidConfig, "io.mode must be file|tail"};
  if (cfg.io.start_position != "begin" && cfg.io.start_position != "end" &&
      cfg.io.start_position != "checkpoint")
    return Error{ErrorCode::kInvalidConfig, "invalid io.start_position"};
  if (cfg.io.rotate_handling != "reopen" && cfg.io.rotate_handling != "error")
    return Error{ErrorCode::kInvalidConfig, "invalid io.rotate_handling"};
  if (cfg.io.max_corrupt_ratio < 0.0F || cfg.io.max_corrupt_ratio > 1.0F)
    return Error{ErrorCode::kInvalidConfig, "io.max_corrupt_ratio must be in [0,1]"};
  if (cfg.io.poll_interval_ms <= 0 || cfg.io.max_consecutive_errors <= 0)
    return Error{ErrorCode::kInvalidConfig, "io polling/error thresholds must be >0"};
  if (cfg.dsp.resampling.method != "linear" && cfg.dsp.resampling.method != "nearest")
    return Error{ErrorCode::kInvalidConfig, "dsp.resampling.method unsupported"};
  if (cfg.dsp.outlier.method != "mad" && cfg.dsp.outlier.method != "hampel")
    return Error{ErrorCode::kInvalidConfig, "dsp.outlier.method unsupported"};
  if (cfg.dsp.outlier.window < 3)
    return Error{ErrorCode::kInvalidConfig, "dsp.outlier.window must be >=3"};

  if (cfg.dsp.window_frames < 16) {
    return Error{ErrorCode::kInvalidConfig, "dsp.window_frames must be >= 16"};
  }
  if (cfg.runtime.ring_buffer_capacity_frames < 8) {
    return Error{ErrorCode::kInvalidConfig, "runtime.ring_buffer_capacity_frames must be >= 8"};
  }
  if (cfg.runtime.max_batch_frames > cfg.runtime.ring_buffer_capacity_frames) {
    return Error{ErrorCode::kInvalidConfig, "runtime.max_batch_frames must be <= capacity"};
  }
  if (detected_subcarrier_count > 0 &&
      (cfg.dsp.topk_subcarriers < 1 || cfg.dsp.topk_subcarriers > detected_subcarrier_count)) {
    return Error{ErrorCode::kInvalidConfig, "dsp.topk_subcarriers out of range"};
  }
  if (cfg.decision.threshold_off >= cfg.decision.threshold_on) {
    return Error{ErrorCode::kInvalidConfig, "decision.threshold_off must be < threshold_on"};
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
  std::stringstream ss;
  ss << in.rdbuf();
  const std::string text = ss.str();

  Config cfg;
  ExtractOptional(text, "config_version", cfg.config_version);
  ExtractOptional(text, "format", cfg.io.format);
  ExtractOptional(text, "path", cfg.io.path);
  ExtractOptional(text, "mode", cfg.io.mode);
  ExtractOptional(text, "checkpoint_path", cfg.io.checkpoint_path);
  ExtractOptional(text, "start_position", cfg.io.start_position);
  ExtractOptional(text, "rotate_handling", cfg.io.rotate_handling);
  ExtractOptional(text, "max_corrupt_ratio", cfg.io.max_corrupt_ratio);
  { int v=0; if (ExtractOptional(text, "max_partial_line_bytes", v)) cfg.io.max_partial_line_bytes=static_cast<std::size_t>(v); }
  ExtractOptional(text, "poll_interval_ms", cfg.io.poll_interval_ms);
  ExtractOptional(text, "max_consecutive_errors", cfg.io.max_consecutive_errors);

  { int v=0; if (ExtractOptional(text, "window_frames", v)) cfg.dsp.window_frames=static_cast<std::size_t>(v); }
  { int v=0; if (ExtractOptional(text, "topk_subcarriers", v)) cfg.dsp.topk_subcarriers=static_cast<std::size_t>(v); }
  ExtractOptional(text, "type", cfg.dsp.smoothing.type);
  ExtractOptional(text, "alpha", cfg.dsp.smoothing.alpha);
  ExtractOptional(text, "kernel", cfg.dsp.smoothing.kernel);
  ExtractOptional(text, "window", cfg.dsp.fft.window);
  ExtractOptional(text, "zero_pad_pow2", cfg.dsp.fft.zero_pad_pow2);
  ExtractOptional(text, "method", cfg.dsp.resampling.method);
  ExtractOptional(text, "reject_jitter_ratio", cfg.dsp.resampling.reject_jitter_ratio);
  ExtractOptional(text, "k", cfg.dsp.outlier.k);
  ExtractOptional(text, "outlier_window", cfg.dsp.outlier.window);
  ExtractOptional(text, "low_hz", cfg.dsp.bands.motion.low_hz);
  ExtractOptional(text, "high_hz", cfg.dsp.bands.motion.high_hz);

  ExtractOptional(text, "threshold_on", cfg.decision.threshold_on);
  ExtractOptional(text, "threshold_off", cfg.decision.threshold_off);
  ExtractOptional(text, "hold_frames", cfg.decision.hold_frames);
  { int v=0; if (ExtractOptional(text, "ring_buffer_capacity_frames", v)) cfg.runtime.ring_buffer_capacity_frames=static_cast<std::size_t>(v); }
  { int v=0; if (ExtractOptional(text, "max_batch_frames", v)) cfg.runtime.max_batch_frames=static_cast<std::size_t>(v); }
  ExtractOptional(text, "clock", cfg.runtime.clock);
  ExtractOptional(text, "max_jitter_ratio", cfg.runtime.max_jitter_ratio);
  ExtractOptional(text, "backpressure", cfg.runtime.backpressure);
  ExtractOptional(text, "report_every_seconds", cfg.runtime.report_every_seconds);
  ExtractOptional(text, "level", cfg.logging.level);

  auto valid = ValidateConfig(cfg, false);
  if (!valid.ok())
    return valid.error();
  return cfg;
}

} // namespace aethersense
