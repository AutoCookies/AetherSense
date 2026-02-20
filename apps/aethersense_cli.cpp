#include <iostream>
#include <string>

#include "aethersense/core/config.hpp"
#include "aethersense/io/csi_reader.hpp"
#include "aethersense/runtime/metrics.hpp"
#include "aethersense/runtime/pipeline.hpp"

int main(int argc, char **argv) {
  std::string config_path;
  std::string input_override;
  std::string format_override;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--input" && i + 1 < argc) {
      input_override = argv[++i];
    } else if (arg == "--format" && i + 1 < argc) {
      format_override = argv[++i];
    }
  }

  if (config_path.empty()) {
    std::cerr << "--config is required\n";
    return 2;
  }

  auto config = aethersense::LoadConfigFromJsonFile(config_path);
  if (!config.ok()) {
    std::cerr << "Config error: " << config.error().message << "\n";
    return 3;
  }

  auto cfg = config.value();
  if (!input_override.empty())
    cfg.io.path = input_override;
  if (!format_override.empty())
    cfg.io.format = format_override;

  auto valid = aethersense::ValidateConfig(cfg, true);
  if (!valid.ok()) {
    std::cerr << "Config validation error: " << valid.error().message << "\n";
    return 4;
  }

  auto reader = aethersense::CreateReader(cfg.io.format, cfg.io.path);
  if (!reader.ok()) {
    std::cerr << "Reader error: " << reader.error().message << "\n";
    return 5;
  }

  aethersense::Pipeline pipeline(cfg.pipeline.presence_threshold, cfg.pipeline.enabled_stages);
  aethersense::StreamMetrics metrics;
  constexpr std::size_t kReportEvery = 2;

  while (true) {
    auto frame_result = reader.value()->next();
    if (!frame_result.ok()) {
      std::cerr << "Read error: " << frame_result.error().message << "\n";
      return 6;
    }
    if (!frame_result.value().has_value()) {
      break;
    }

    auto decision = pipeline.Process(*frame_result.value());
    if (!decision.ok()) {
      std::cerr << "Pipeline error: " << decision.error().message << "\n";
      return 7;
    }

    ++metrics.frames;
    metrics.energy_sum += decision.value().energy;
    if (decision.value().present) {
      ++metrics.present_count;
    }

    if (metrics.frames % kReportEvery == 0) {
      const double avg_energy = metrics.energy_sum / static_cast<double>(metrics.frames);
      const double present_rate =
          static_cast<double>(metrics.present_count) / static_cast<double>(metrics.frames);
      std::cout << "frames=" << metrics.frames << " avg_energy=" << avg_energy
                << " present_rate=" << present_rate << "\n";
    }
  }

  if (metrics.frames > 0) {
    const double avg_energy = metrics.energy_sum / static_cast<double>(metrics.frames);
    const double present_rate =
        static_cast<double>(metrics.present_count) / static_cast<double>(metrics.frames);
    std::cout << "final frames=" << metrics.frames << " avg_energy=" << avg_energy
              << " present_rate=" << present_rate << "\n";
  }

  return 0;
}
