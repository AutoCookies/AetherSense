#include "aethersense/io/csi_reader.hpp"

#include <memory>

#include "aethersense/io/record_recovery.hpp"

namespace aethersense {
namespace {

class RecoveryReader final : public ICsiReader {
public:
  RecoveryReader(const Config::Io &cfg, std::unique_ptr<io::IStreamReader> stream)
      : cfg_(cfg), stream_(std::move(stream)) {}

  Result<std::optional<CsiFrame>> next() override {
    while (true) {
      auto rec = stream_->read_next();
      if (!rec.ok()) {
        ++stats_.consecutive_errors_current;
        if (stats_.consecutive_errors_current > static_cast<std::size_t>(cfg_.max_consecutive_errors)) {
          return rec.error();
        }
        continue;
      }
      if (rec.value().eof) {
        return std::optional<CsiFrame>{};
      }
      if (rec.value().line.empty()) {
        return std::optional<CsiFrame>{};
      }

      auto parsed = (cfg_.format == "csv") ? io::ParseCsvRecord(rec.value().line)
                                             : io::ParseJsonlRecord(rec.value().line);
      if (parsed.corrupt) {
        ++stats_.records_corrupt_total;
        ++corrupt_window_;
        ++window_size_;
        if (window_size_ >= 64) {
          const float ratio = static_cast<float>(corrupt_window_) / static_cast<float>(window_size_);
          window_size_ = 0;
          corrupt_window_ = 0;
          if (ratio > cfg_.max_corrupt_ratio) {
            return Error{ErrorCode::kParseError, "corrupt ratio exceeded"};
          }
        }
        continue;
      }

      ++stats_.records_total;
      ++window_size_;
      stats_.consecutive_errors_current = 0;
      return std::optional<CsiFrame>{*parsed.frame};
    }
  }

  io::StreamStats stream_stats() const override {
    auto s = stream_->stats();
    s.records_corrupt_total += stats_.records_corrupt_total;
    s.records_total += stats_.records_total;
    s.consecutive_errors_current = stats_.consecutive_errors_current;
    return s;
  }

private:
  Config::Io cfg_;
  std::unique_ptr<io::IStreamReader> stream_;
  io::StreamStats stats_;
  std::size_t corrupt_window_{0};
  std::size_t window_size_{0};
};

} // namespace

Result<std::unique_ptr<ICsiReader>> CreateReader(const Config::Io &io_cfg, const std::string &path) {
  auto stream = io::CreateStreamReader(io_cfg);
  if (!stream.ok()) {
    return stream.error();
  }
  auto opened = stream.value()->open(path);
  if (!opened.ok()) {
    return opened.error();
  }
  return std::unique_ptr<ICsiReader>(new RecoveryReader(io_cfg, std::move(stream.value())));
}

} // namespace aethersense
