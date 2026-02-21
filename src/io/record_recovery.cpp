#include "aethersense/io/record_recovery.hpp"

#include <complex>
#include <sstream>
#include <vector>

namespace aethersense::io {
namespace {
std::vector<std::string> Split(const std::string &value, char delim) {
  std::stringstream ss(value);
  std::string part;
  std::vector<std::string> out;
  while (std::getline(ss, part, delim)) {
    out.push_back(part);
  }
  return out;
}

bool ParseFloatList(const std::string &value, char delim, std::vector<float> &out) {
  for (const auto &token : Split(value, delim)) {
    try {
      out.push_back(std::stof(token));
    } catch (...) {
      return false;
    }
  }
  return true;
}
} // namespace

RecoveryResult ParseCsvRecord(const std::string &line) {
  const auto cols = Split(line, ',');
  if (cols.size() != 7) {
    return {.corrupt = true, .error = "CSV line must have 7 columns"};
  }
  CsiFrame frame;
  try {
    frame.timestamp_ns = static_cast<std::uint64_t>(std::stoull(cols[0]));
    frame.center_freq_hz = static_cast<std::uint64_t>(std::stoull(cols[1]));
    frame.rx_count = static_cast<std::uint8_t>(std::stoi(cols[2]));
    frame.tx_count = static_cast<std::uint8_t>(std::stoi(cols[3]));
    frame.subcarrier_count = static_cast<std::uint16_t>(std::stoi(cols[4]));
  } catch (...) {
    return {.corrupt = true, .error = "invalid numeric field"};
  }

  std::vector<float> re;
  std::vector<float> im;
  if (!ParseFloatList(cols[5], ';', re) || !ParseFloatList(cols[6], ';', im)) {
    return {.corrupt = true, .error = "invalid float token"};
  }

  const std::size_t expected =
      static_cast<std::size_t>(frame.rx_count) * frame.tx_count * frame.subcarrier_count;
  if (re.size() != expected || im.size() != expected) {
    return {.corrupt = true, .error = "data_re/data_im length mismatch"};
  }

  frame.data.reserve(expected);
  for (std::size_t i = 0; i < expected; ++i) {
    frame.data.emplace_back(re[i], im[i]);
  }
  return {.frame = std::move(frame)};
}

RecoveryResult ParseJsonlRecord(const std::string &line) {
  auto grab = [&](const std::string &key) -> std::optional<std::string> {
    const auto p = line.find("\"" + key + "\"");
    if (p == std::string::npos)
      return std::nullopt;
    const auto c = line.find(':', p);
    if (c == std::string::npos)
      return std::nullopt;
    auto end = line.find_first_of(",}", c + 1);
    if (end == std::string::npos)
      end = line.size();
    return line.substr(c + 1, end - c - 1);
  };
  auto array = [&](const std::string &key) -> std::optional<std::string> {
    const auto p = line.find("\"" + key + "\"");
    if (p == std::string::npos)
      return std::nullopt;
    const auto b = line.find('[', p);
    const auto e = line.find(']', b);
    if (b == std::string::npos || e == std::string::npos)
      return std::nullopt;
    return line.substr(b + 1, e - b - 1);
  };

  CsiFrame frame;
  try {
    frame.timestamp_ns = std::stoull(*grab("timestamp_ns"));
    frame.center_freq_hz = std::stoull(*grab("center_freq_hz"));
    frame.rx_count = static_cast<std::uint8_t>(std::stoi(*grab("rx")));
    frame.tx_count = static_cast<std::uint8_t>(std::stoi(*grab("tx")));
    frame.subcarrier_count = static_cast<std::uint16_t>(std::stoi(*grab("subcarrier_count")));
  } catch (...) {
    return {.corrupt = true, .error = "JSONL numeric parse failure"};
  }

  std::vector<float> re;
  std::vector<float> im;
  if (!array("data_re").has_value() || !array("data_im").has_value()) {
    return {.corrupt = true, .error = "JSONL missing arrays"};
  }
  if (!ParseFloatList(*array("data_re"), ',', re) || !ParseFloatList(*array("data_im"), ',', im)) {
    return {.corrupt = true, .error = "JSONL array parse failure"};
  }

  const std::size_t expected =
      static_cast<std::size_t>(frame.rx_count) * frame.tx_count * frame.subcarrier_count;
  if (re.size() != expected || im.size() != expected) {
    return {.corrupt = true, .error = "JSONL data length mismatch"};
  }
  frame.data.reserve(expected);
  for (std::size_t i = 0; i < expected; ++i) {
    frame.data.emplace_back(re[i], im[i]);
  }
  return {.frame = std::move(frame)};
}

} // namespace aethersense::io
