#pragma once

#include <string>
#include <utility>

namespace aethersense {

enum class ErrorCode {
  kOk = 0,
  kInvalidArgument,
  kInvalidConfig,
  kUnsupportedFormat,
  kIoError,
  kParseError,
  kTimeout,
};

struct Error {
  ErrorCode code{ErrorCode::kOk};
  std::string message{};
};

template <typename T> class Result {
public:
  Result(const T &value) : ok_(true), value_(value) {}
  Result(T &&value) : ok_(true), value_(std::move(value)) {}
  Result(Error error) : ok_(false), error_(std::move(error)) {}

  [[nodiscard]] bool ok() const { return ok_; }
  [[nodiscard]] const T &value() const { return value_; }
  [[nodiscard]] T &value() { return value_; }
  [[nodiscard]] const Error &error() const { return error_; }

private:
  bool ok_{false};
  T value_{};
  Error error_{};
};

} // namespace aethersense
