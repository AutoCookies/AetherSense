#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include "aethersense/core/errors.hpp"

namespace aethersense {

template <typename T> class RingBuffer {
public:
  explicit RingBuffer(std::size_t capacity)
      : capacity_(capacity), buffer_(capacity), head_(0), tail_(0), size_(0) {}

  void push(T &&item) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (size_ == capacity_) {
        tail_ = (tail_ + 1) % capacity_;
        --size_;
      }
      buffer_[head_] = std::move(item);
      head_ = (head_ + 1) % capacity_;
      ++size_;
    }
    cv_.notify_one();
  }

  bool try_pop(T &out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == 0) {
      return false;
    }
    out = std::move(*buffer_[tail_]);
    buffer_[tail_].reset();
    tail_ = (tail_ + 1) % capacity_;
    --size_;
    return true;
  }

  Result<T> pop_blocking(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this] { return size_ > 0; })) {
      return Error{ErrorCode::kTimeout, "RingBuffer pop timeout"};
    }
    T out = std::move(*buffer_[tail_]);
    buffer_[tail_].reset();
    tail_ = (tail_ + 1) % capacity_;
    --size_;
    return out;
  }

  [[nodiscard]] std::size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
  }

  [[nodiscard]] std::size_t capacity() const { return capacity_; }

private:
  const std::size_t capacity_;
  std::vector<std::optional<T>> buffer_;
  std::size_t head_;
  std::size_t tail_;
  std::size_t size_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace aethersense
