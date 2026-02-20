#pragma once

#include <cstddef>
#include <string>

#include "aethersense/core/errors.hpp"

namespace aethersense {

struct Config {
  int config_version{2};

  struct Io {
    std::string format{"csv"};
    std::string path{};
  } io;

  struct Dsp {
    std::size_t window_frames{32};
    std::size_t topk_subcarriers{1};

    struct Smoothing {
      std::string type{"ema"};
      float alpha{0.3F};
      int kernel{3};
    } smoothing;

    struct Fft {
      std::string window{"hann"};
      bool zero_pad_pow2{true};
    } fft;

    struct Bands {
      struct Range {
        float low_hz{0.5F};
        float high_hz{5.0F};
      } motion;

      struct BreathingRange {
        bool enabled{false};
        float low_hz{0.1F};
        float high_hz{0.5F};
      } breathing;
    } bands;
  } dsp;

  struct Decision {
    float threshold_on{0.2F};
    float threshold_off{0.1F};
    int hold_frames{3};
  } decision;

  struct Runtime {
    std::size_t ring_buffer_capacity_frames{64};
    std::size_t max_batch_frames{16};
    std::string clock{"from_input"};
    float max_jitter_ratio{0.2F};
    std::string backpressure{"drop_oldest"};
    int report_every_seconds{1};
  } runtime;

  struct Logging {
    std::string level{"info"};
  } logging;
};

Result<Config> LoadConfigFromJsonFile(const std::string &path);
Result<bool> ValidateConfig(const Config &cfg, bool require_existing_path,
                            std::size_t detected_subcarrier_count = 0);

} // namespace aethersense
