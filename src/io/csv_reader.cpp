#include "aethersense/io/csi_reader.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace aethersense {
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

Result<std::vector<float>> ParseFloatList(const std::string &value, char delim) {
  std::vector<float> out;
  for (const auto &token : Split(value, delim)) {
    try {
      out.push_back(std::stof(token));
    } catch (const std::exception &ex) {
      return Error{ErrorCode::kParseError, std::string("invalid float token: ") + ex.what()};
    }
  }
  return out;
}

class CsvCsiReader final : public ICsiReader {
public:
  explicit CsvCsiReader(const std::string &path) : in_(path) {
    if (in_) {
      std::getline(in_, header_);
    }
  }

  Result<std::optional<CsiFrame>> next() override {
    if (!in_) {
      return Error{ErrorCode::kIoError, "CSV file not opened"};
    }
    std::string line;
    if (!std::getline(in_, line)) {
      return std::optional<CsiFrame>{};
    }
    const auto cols = Split(line, ',');
    if (cols.size() != 7) {
      return Error{ErrorCode::kParseError, "CSV line must have 7 columns"};
    }

    CsiFrame frame;
    try {
      frame.timestamp_ns = static_cast<std::uint64_t>(std::stoull(cols[0]));
      frame.center_freq_hz = static_cast<std::uint64_t>(std::stoull(cols[1]));
      frame.rx_count = static_cast<std::uint8_t>(std::stoi(cols[2]));
      frame.tx_count = static_cast<std::uint8_t>(std::stoi(cols[3]));
      frame.subcarrier_count = static_cast<std::uint16_t>(std::stoi(cols[4]));
    } catch (const std::exception &ex) {
      return Error{ErrorCode::kParseError, std::string("invalid numeric field: ") + ex.what()};
    }

    auto re = ParseFloatList(cols[5], ';');
    if (!re.ok())
      return re.error();
    auto im = ParseFloatList(cols[6], ';');
    if (!im.ok())
      return im.error();

    const std::size_t expected =
        static_cast<std::size_t>(frame.rx_count) * frame.tx_count * frame.subcarrier_count;
    if (re.value().size() != expected || im.value().size() != expected) {
      return Error{ErrorCode::kParseError, "data_re/data_im length mismatch"};
    }

    frame.data.reserve(expected);
    for (std::size_t i = 0; i < expected; ++i) {
      frame.data.emplace_back(re.value()[i], im.value()[i]);
    }
    return std::optional<CsiFrame>{std::move(frame)};
  }

private:
  std::ifstream in_;
  std::string header_;
};

} // namespace

Result<std::unique_ptr<ICsiReader>> CreateCsvReader(const std::string &path) {
  return std::unique_ptr<ICsiReader>(new CsvCsiReader(path));
}

} // namespace aethersense
