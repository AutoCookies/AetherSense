#include "aethersense/io/stream_reader.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace aethersense::io {
namespace fs = std::filesystem;

class FileStreamReader final : public IStreamReader {
public:
  explicit FileStreamReader(const Config::Io &cfg) : cfg_(cfg) {}

  Result<bool> open(const std::string &path) override {
    path_ = path;
    in_.close();
    in_.open(path_, std::ios::in);
    if (!in_) {
      return Error{ErrorCode::kIoError, "failed to open stream: " + path_};
    }
    partial_.clear();
    offset_ = 0;
    signature_ = Signature(path_);
    ResumeIfCheckpointed();
    return true;
  }

  Result<StreamRecord> read_next() override {
    if (!in_) {
      return Error{ErrorCode::kIoError, "stream not opened"};
    }
    DetectRotate();

    std::string line;
    if (std::getline(in_, line)) {
      offset_ = static_cast<std::uint64_t>(in_.tellg());
      if (!partial_.empty()) {
        line = partial_ + line;
        partial_.clear();
      }
      ++stats_.records_total;
      stats_.consecutive_errors_current = 0;
      checkpoint_timestamp_ = 0;
      WriteCheckpoint();
      return StreamRecord{line, false};
    }

    if (!in_.eof()) {
      ++stats_.consecutive_errors_current;
      return Error{ErrorCode::kIoError, "stream read failure"};
    }

    in_.clear();
    if (cfg_.mode == "tail") {
      std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.poll_interval_ms));
      DetectRotate();
      return StreamRecord{"", false};
    }
    return StreamRecord{"", true};
  }

  StreamStats stats() const override { return stats_; }
  std::uint64_t last_timestamp_ns() const override { return checkpoint_timestamp_; }

  void OnCorrupt() {
    ++stats_.records_corrupt_total;
    ++stats_.consecutive_errors_current;
  }

  void OnPartial(std::string chunk) {
    partial_ += std::move(chunk);
    ++stats_.records_partial_total;
    if (partial_.size() > cfg_.max_partial_line_bytes) {
      partial_.clear();
      ++stats_.records_corrupt_total;
    }
  }

  void SetTimestamp(std::uint64_t ts) {
    checkpoint_timestamp_ = ts;
    WriteCheckpoint();
  }

private:
  std::string Signature(const std::string &p) {
    if (!fs::exists(p))
      return {};
    auto s = fs::status(p);
    auto fsz = fs::file_size(p);
    return std::to_string(static_cast<int>(s.type())) + ":" + std::to_string(fsz);
  }

  void ResumeIfCheckpointed() {
    if (cfg_.start_position == "end") {
      in_.seekg(0, std::ios::end);
      offset_ = static_cast<std::uint64_t>(in_.tellg());
      return;
    }
    std::ifstream ck(cfg_.checkpoint_path);
    if (!ck || cfg_.start_position != "checkpoint")
      return;
    std::string sig;
    ck >> sig >> offset_ >> checkpoint_timestamp_;
    if (sig == signature_) {
      in_.seekg(static_cast<std::streamoff>(offset_), std::ios::beg);
      ++stats_.checkpoint_resume_total;
    }
  }

  void WriteCheckpoint() {
    std::ofstream ck(cfg_.checkpoint_path, std::ios::trunc);
    if (!ck)
      return;
    ck << signature_ << ' ' << offset_ << ' ' << checkpoint_timestamp_;
    ++stats_.checkpoint_writes_total;
  }

  void DetectRotate() {
    if (!fs::exists(path_))
      return;
    const auto new_sig = Signature(path_);
    if (new_sig != signature_) {
      ++stats_.rotations_detected_total;
      if (cfg_.rotate_handling == "reopen") {
        open(path_);
      }
    }
  }

  Config::Io cfg_;
  std::string path_;
  std::ifstream in_;
  std::string partial_;
  std::uint64_t offset_{0};
  std::uint64_t checkpoint_timestamp_{0};
  std::string signature_;
  StreamStats stats_;
};

Result<std::unique_ptr<IStreamReader>> CreateStreamReader(const Config::Io &cfg) {
  return std::unique_ptr<IStreamReader>(new FileStreamReader(cfg));
}

} // namespace aethersense::io
