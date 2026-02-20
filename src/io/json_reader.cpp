#include "aethersense/io/csi_reader.hpp"

#include <cstdint>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace aethersense {

Result<std::unique_ptr<ICsiReader>> CreateCsvReader(const std::string &path);

namespace {

Result<std::string> ExtractField(const std::string &line, const std::string &key) {
  const std::regex re("\"" + key + "\"\\s*:\\s*([^,}]+)");
  std::smatch m;
  if (!std::regex_search(line, m, re)) {
    return Error{ErrorCode::kParseError, "missing key: " + key};
  }
  return m[1].str();
}

Result<std::vector<float>> ExtractFloatArray(const std::string &line, const std::string &key) {
  const std::regex re("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
  std::smatch m;
  if (!std::regex_search(line, m, re)) {
    return Error{ErrorCode::kParseError, "missing array: " + key};
  }

  std::vector<float> out;
  std::stringstream ss(m[1].str());
  std::string token;
  while (std::getline(ss, token, ',')) {
    out.push_back(std::stof(token));
  }
  return out;
}

class JsonlCsiReader final : public ICsiReader {
public:
  explicit JsonlCsiReader(const std::string &path) : in_(path) {}

  Result<std::optional<CsiFrame>> next() override {
    if (!in_) {
      return Error{ErrorCode::kIoError, "JSONL file not opened"};
    }
    std::string line;
    if (!std::getline(in_, line)) {
      return std::optional<CsiFrame>{};
    }

    try {
      CsiFrame frame;
      frame.timestamp_ns = std::stoull(ExtractField(line, "timestamp_ns").value());
      frame.center_freq_hz = std::stoull(ExtractField(line, "center_freq_hz").value());
      frame.rx_count = static_cast<std::uint8_t>(std::stoi(ExtractField(line, "rx").value()));
      frame.tx_count = static_cast<std::uint8_t>(std::stoi(ExtractField(line, "tx").value()));
      frame.subcarrier_count =
          static_cast<std::uint16_t>(std::stoi(ExtractField(line, "subcarrier_count").value()));
      auto re = ExtractFloatArray(line, "data_re");
      if (!re.ok())
        return re.error();
      auto im = ExtractFloatArray(line, "data_im");
      if (!im.ok())
        return im.error();

      const std::size_t expected =
          static_cast<std::size_t>(frame.rx_count) * frame.tx_count * frame.subcarrier_count;
      if (re.value().size() != expected || im.value().size() != expected) {
        return Error{ErrorCode::kParseError, "JSONL data_re/data_im length mismatch"};
      }
      frame.data.reserve(expected);
      for (std::size_t i = 0; i < expected; ++i) {
        frame.data.emplace_back(re.value()[i], im.value()[i]);
      }
      return std::optional<CsiFrame>{std::move(frame)};
    } catch (const std::exception &ex) {
      return Error{ErrorCode::kParseError, std::string("JSONL parse failure: ") + ex.what()};
    }
  }

private:
  std::ifstream in_;
};

} // namespace

Result<std::unique_ptr<ICsiReader>> CreateReader(const std::string &format,
                                                 const std::string &path) {
  if (format == "csv") {
    return CreateCsvReader(path);
  }
  if (format == "jsonl") {
    return std::unique_ptr<ICsiReader>(new JsonlCsiReader(path));
  }
  return Error{ErrorCode::kUnsupportedFormat, "Unsupported format: " + format};
}

} // namespace aethersense
