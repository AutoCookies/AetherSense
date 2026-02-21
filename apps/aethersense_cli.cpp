#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "aethersense/core/config.hpp"
#include "aethersense/core/version.hpp"
#include "aethersense/io/csi_reader.hpp"
#include "aethersense/runtime/metrics.hpp"
#include "aethersense/runtime/pipeline.hpp"

namespace {

void PrintSchema() {
  std::cout << "AetherSense config v3 schema with io tail/checkpoint and resampling/outlier controls\n";
}

} // namespace

int main(int argc, char **argv) {
  std::string config_path;
  std::string input_override;
  std::string format_override;
  std::string export_path;
  bool dry_run = false;
  bool output_jsonl = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--input" && i + 1 < argc) {
      input_override = argv[++i];
    } else if (arg == "--format" && i + 1 < argc) {
      format_override = argv[++i];
    } else if (arg == "--export-decisions" && i + 1 < argc) {
      export_path = argv[++i];
    } else if (arg == "--output" && i + 1 < argc) {
      output_jsonl = std::string(argv[++i]) == "jsonl";
    } else if (arg == "--print-config-schema") {
      PrintSchema();
      return 0;
    } else if (arg == "--dry-run") {
      dry_run = true;
    } else if (arg == "--version") {
      std::cout << aethersense::kVersion << "\n";
      return 0;
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
  if (!input_override.empty()) {
    cfg.io.path = input_override;
  }
  if (!format_override.empty()) {
    cfg.io.format = format_override;
  }

  auto valid = aethersense::ValidateConfig(cfg, true);
  if (!valid.ok()) {
    std::cerr << "Config validation error: " << valid.error().message << "\n";
    return 4;
  }

  auto reader = aethersense::CreateReader(cfg.io, cfg.io.path);
  if (!reader.ok()) {
    std::cerr << "Reader error: " << reader.error().message << "\n";
    return 5;
  }

  if (dry_run) {
    std::cout << "dry-run ok\n";
    return 0;
  }

  std::ofstream export_file;
  if (!export_path.empty()) {
    export_file.open(export_path);
    export_file << "timestamp_ns,energy_motion,energy_breathing,present\n";
  }

  aethersense::Pipeline pipeline(cfg);
  aethersense::RuntimeMetrics metrics;

  std::size_t decisions_total = 0;
  std::size_t present_total = 0;
  double energy_sum = 0.0;
  const auto report_start = std::chrono::steady_clock::now();

  while (true) {
    auto frame_result = reader.value()->next();
    if (!frame_result.ok()) {
      std::cerr << "Read error: " << frame_result.error().message << "\n";
      return 6;
    }
    if (!frame_result.value().has_value()) {
      if (cfg.io.mode == "tail") {
        continue;
      }
      break;
    }

    ++metrics.frames_read_total;
    auto decision = pipeline.ProcessFrame(*frame_result.value(), metrics);
    if (!decision.ok()) {
      std::cerr << "Pipeline error: " << decision.error().message << "\n";
      return 7;
    }

    if (decision.value().has_value()) {
      ++decisions_total;
      energy_sum += decision.value()->energy_motion;
      if (decision.value()->present) {
        ++present_total;
      }
      const auto total_s =
          std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - report_start).count();
      const double fps = total_s > 0 ? metrics.frames_read_total / total_s : 0.0;
      if (output_jsonl) {
        const auto st = reader.value()->stream_stats();
        std::cout << "{\"timestamp_ns\":" << decision.value()->timestamp_ns
                  << ",\"energy_motion\":" << decision.value()->energy_motion
                  << ",\"present\":" << (decision.value()->present ? "true" : "false")
                  << ",\"fps\":" << fps << ",\"p50_us\":" << metrics.Percentile(50)
                  << ",\"p95_us\":" << metrics.Percentile(95)
                  << ",\"drops\":" << metrics.frames_dropped_total
                  << ",\"corrupt\":" << st.records_corrupt_total << "}\n";
      }
      if (export_file) {
        export_file << decision.value()->timestamp_ns << ',' << decision.value()->energy_motion
                    << ',' << decision.value()->energy_breathing << ','
                    << (decision.value()->present ? 1 : 0) << '\n';
      }
    }
  }

  if (!output_jsonl && decisions_total > 0) {
    const double present_ratio =
        static_cast<double>(present_total) / static_cast<double>(decisions_total);
    const double avg_energy = energy_sum / static_cast<double>(decisions_total);
    std::cout << "final decisions=" << decisions_total << " present_ratio=" << present_ratio
              << " energy_motion=" << avg_energy
              << " windows_rejected=" << metrics.windows_rejected_total << "\n";
  }

  return 0;
}
