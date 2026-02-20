#pragma once

#include <complex>
#include <cstdint>
#include <span>
#include <vector>

namespace aethersense {

struct CsiFrame {
  std::uint64_t timestamp_ns{0};
  std::uint64_t center_freq_hz{0};
  std::uint16_t subcarrier_count{0};
  std::uint8_t rx_count{0};
  std::uint8_t tx_count{0};
  std::vector<std::complex<float>> data{};

  [[nodiscard]] std::size_t sample_count() const { return data.size(); }
};

struct FrameView {
  std::uint64_t timestamp_ns{0};
  std::uint64_t center_freq_hz{0};
  std::uint16_t subcarrier_count{0};
  std::uint8_t rx_count{0};
  std::uint8_t tx_count{0};
  std::span<const std::complex<float>> data{};
};

inline FrameView MakeFrameView(const CsiFrame &frame) {
  return FrameView{frame.timestamp_ns,     frame.center_freq_hz,
                   frame.subcarrier_count, frame.rx_count,
                   frame.tx_count,         std::span<const std::complex<float>>(frame.data)};
}

inline std::size_t FlatIndex(std::uint8_t rx, std::uint8_t tx, std::uint16_t sc,
                             std::uint8_t tx_count, std::uint16_t subcarrier_count) {
  return (static_cast<std::size_t>(rx) * tx_count + tx) * subcarrier_count + sc;
}

} // namespace aethersense
