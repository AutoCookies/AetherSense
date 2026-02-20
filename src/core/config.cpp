#include "aethersense/core/config.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace aethersense {
namespace {

Result<std::string> ExtractString(const std::string &text, const std::string &key) {
  const std::regex regex("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
  std::smatch match;
  if (!std::regex_search(text, match, regex)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return match[1].str();
}

Result<int> ExtractInt(const std::string &text, const std::string &key) {
  const std::regex regex("\"" + key + "\"\\s*:\\s*([0-9]+)");
  std::smatch match;
  if (!std::regex_search(text, match, regex)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return std::stoi(match[1].str());
}

Result<float> ExtractFloat(const std::string &text, const std::string &key) {
  const std::regex regex("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
  std::smatch match;
  if (!std::regex_search(text, match, regex)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return std::stof(match[1].str());
}

Result<bool> ExtractBool(const std::string &text, const std::string &key) {
  const std::regex regex("\"" + key + "\"\\s*:\\s*(true|false)");
  std::smatch match;
  if (!std::regex_search(text, match, regex)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return match[1].str() == "true";
}

bool IsSupportedFormat(const std::string &format) { return format == "csv" || format == "jsonl"; }

bool IsSupportedBackpressure(const std::string &policy) {
  return policy == "block" || policy == "drop_oldest" || policy == "drop_newest";
}

} // namespace

Result<bool> ValidateConfig(const Config &cfg, bool require_existing_path,
                            std::size_t detected_subcarrier_count) {
  if (cfg.config_version != 2) {
    return Error{ErrorCode::kInvalidConfig, "config_version must be 2"};
  }
  if (!IsSupportedFormat(cfg.io.format)) {
    return Error{ErrorCode::kUnsupportedFormat, "io.format must be csv or jsonl"};
  }
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
  if (cfg.decision.hold_frames < 0) {
    return Error{ErrorCode::kInvalidConfig, "decision.hold_frames must be >= 0"};
  }
  if (cfg.runtime.max_jitter_ratio < 0.0F || cfg.runtime.max_jitter_ratio > 1.0F) {
    return Error{ErrorCode::kInvalidConfig, "runtime.max_jitter_ratio must be in [0,1]"};
  }
  if (!IsSupportedBackpressure(cfg.runtime.backpressure)) {
    return Error{ErrorCode::kInvalidConfig, "runtime.backpressure unsupported"};
  }
  if (cfg.runtime.report_every_seconds <= 0) {
    return Error{ErrorCode::kInvalidConfig, "runtime.report_every_seconds must be > 0"};
  }
  if (cfg.dsp.bands.motion.low_hz <= 0.0F ||
      cfg.dsp.bands.motion.high_hz <= cfg.dsp.bands.motion.low_hz) {
    return Error{ErrorCode::kInvalidConfig, "invalid motion band"};
  }
  if (cfg.dsp.bands.breathing.enabled &&
      (cfg.dsp.bands.breathing.low_hz <= 0.0F ||
       cfg.dsp.bands.breathing.high_hz <= cfg.dsp.bands.breathing.low_hz)) {
    return Error{ErrorCode::kInvalidConfig, "invalid breathing band"};
  }
  if (cfg.runtime.clock != "monotonic" && cfg.runtime.clock != "from_input") {
    return Error{ErrorCode::kInvalidConfig, "runtime.clock must be monotonic or from_input"};
  }
  if (cfg.dsp.smoothing.type == "ema") {
    if (cfg.dsp.smoothing.alpha <= 0.0F || cfg.dsp.smoothing.alpha > 1.0F) {
      return Error{ErrorCode::kInvalidConfig, "dsp.smoothing.alpha must be in (0,1]"};
    }
  } else if (cfg.dsp.smoothing.type == "median") {
    if (cfg.dsp.smoothing.kernel < 3 || cfg.dsp.smoothing.kernel % 2 == 0) {
      return Error{ErrorCode::kInvalidConfig, "median kernel must be odd and >=3"};
    }
  } else {
    return Error{ErrorCode::kInvalidConfig, "unsupported smoothing type"};
  }
  if (cfg.dsp.fft.window != "hann" && cfg.dsp.fft.window != "hamming") {
    return Error{ErrorCode::kInvalidConfig, "unsupported fft window"};
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
  auto version = ExtractInt(text, "config_version");
  if (!version.ok())
    return version.error();
  cfg.config_version = version.value();
  auto format = ExtractString(text, "format");
  if (!format.ok())
    return format.error();
  cfg.io.format = format.value();
  auto io_path = ExtractString(text, "path");
  if (!io_path.ok())
    return io_path.error();
  cfg.io.path = io_path.value();

  auto window_frames = ExtractInt(text, "window_frames");
  if (!window_frames.ok())
    return window_frames.error();
  cfg.dsp.window_frames = static_cast<std::size_t>(window_frames.value());
  auto topk = ExtractInt(text, "topk_subcarriers");
  if (!topk.ok())
    return topk.error();
  cfg.dsp.topk_subcarriers = static_cast<std::size_t>(topk.value());

  auto smoothing_type = ExtractString(text, "type");
  if (!smoothing_type.ok())
    return smoothing_type.error();
  cfg.dsp.smoothing.type = smoothing_type.value();
  auto alpha = ExtractFloat(text, "alpha");
  if (!alpha.ok())
    return alpha.error();
  cfg.dsp.smoothing.alpha = alpha.value();
  auto kernel = ExtractInt(text, "kernel");
  if (!kernel.ok())
    return kernel.error();
  cfg.dsp.smoothing.kernel = kernel.value();

  auto fft_window = ExtractString(text, "window");
  if (!fft_window.ok())
    return fft_window.error();
  cfg.dsp.fft.window = fft_window.value();
  auto zero_pad = ExtractBool(text, "zero_pad_pow2");
  if (!zero_pad.ok())
    return zero_pad.error();
  cfg.dsp.fft.zero_pad_pow2 = zero_pad.value();

  auto motion_low = ExtractFloat(text, "low_hz");
  if (!motion_low.ok())
    return motion_low.error();
  auto motion_high = ExtractFloat(text, "high_hz");
  if (!motion_high.ok())
    return motion_high.error();
  cfg.dsp.bands.motion.low_hz = motion_low.value();
  cfg.dsp.bands.motion.high_hz = motion_high.value();

  const std::regex breathing_regex(R"("breathing"\s*:\s*\{([^\}]*)\})");
  std::smatch breathing_match;
  if (std::regex_search(text, breathing_match, breathing_regex)) {
    const std::string breathing_text = breathing_match[1].str();
    auto be = ExtractBool(breathing_text, "enabled");
    if (!be.ok())
      return be.error();
    cfg.dsp.bands.breathing.enabled = be.value();
    auto bl = ExtractFloat(breathing_text, "low_hz");
    if (!bl.ok())
      return bl.error();
    auto bh = ExtractFloat(breathing_text, "high_hz");
    if (!bh.ok())
      return bh.error();
    cfg.dsp.bands.breathing.low_hz = bl.value();
    cfg.dsp.bands.breathing.high_hz = bh.value();
  }

  auto threshold_on = ExtractFloat(text, "threshold_on");
  if (!threshold_on.ok())
    return threshold_on.error();
  cfg.decision.threshold_on = threshold_on.value();
  auto threshold_off = ExtractFloat(text, "threshold_off");
  if (!threshold_off.ok())
    return threshold_off.error();
  cfg.decision.threshold_off = threshold_off.value();
  auto hold = ExtractInt(text, "hold_frames");
  if (!hold.ok())
    return hold.error();
  cfg.decision.hold_frames = hold.value();

  auto rb = ExtractInt(text, "ring_buffer_capacity_frames");
  if (!rb.ok())
    return rb.error();
  cfg.runtime.ring_buffer_capacity_frames = static_cast<std::size_t>(rb.value());
  auto batch = ExtractInt(text, "max_batch_frames");
  if (!batch.ok())
    return batch.error();
  cfg.runtime.max_batch_frames = static_cast<std::size_t>(batch.value());
  auto clock = ExtractString(text, "clock");
  if (!clock.ok())
    return clock.error();
  cfg.runtime.clock = clock.value();
  auto jitter = ExtractFloat(text, "max_jitter_ratio");
  if (!jitter.ok())
    return jitter.error();
  cfg.runtime.max_jitter_ratio = jitter.value();
  auto backpressure = ExtractString(text, "backpressure");
  if (!backpressure.ok())
    return backpressure.error();
  cfg.runtime.backpressure = backpressure.value();
  auto report_every = ExtractInt(text, "report_every_seconds");
  if (!report_every.ok())
    return report_every.error();
  cfg.runtime.report_every_seconds = report_every.value();

  auto level = ExtractString(text, "level");
  if (!level.ok())
    return level.error();
  cfg.logging.level = level.value();

  auto valid = ValidateConfig(cfg, false);
  if (!valid.ok()) {
    return valid.error();
  }
  return cfg;
}

} // namespace aethersense
