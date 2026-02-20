#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "aethersense/core/errors.hpp"

namespace aethersense {

enum class BackpressurePolicy { kBlock, kDropOldest, kDropNewest };

template <typename T> class RingBuffer {
public:
  explicit RingBuffer(std::size_t capacity)
      : capacity_(capacity), buffer_(capacity), head_(0), tail_(0), size_(0) {}

  bool push(T &&item, BackpressurePolicy policy,
            std::chrono::milliseconds block_timeout = std::chrono::milliseconds(100)) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (policy == BackpressurePolicy::kBlock) {
      cv_not_full_.wait_for(lock, block_timeout, [this] { return size_ < capacity_; });
      if (size_ == capacity_) {
        return false;
      }
    } else if (policy == BackpressurePolicy::kDropNewest && size_ == capacity_) {
      return false;
    } else if (policy == BackpressurePolicy::kDropOldest && size_ == capacity_) {
      buffer_[tail_].reset();
      tail_ = (tail_ + 1) % capacity_;
      --size_;
    }

    buffer_[head_] = std::move(item);
    head_ = (head_ + 1) % capacity_;
    ++size_;
    lock.unlock();
    cv_not_empty_.notify_one();
    return true;
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
    cv_not_full_.notify_one();
    return true;
  }

  Result<T> pop_blocking(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_not_empty_.wait_for(lock, timeout, [this] { return size_ > 0; })) {
      return Error{ErrorCode::kTimeout, "RingBuffer pop timeout"};
    }
    T out = std::move(*buffer_[tail_]);
    buffer_[tail_].reset();
    tail_ = (tail_ + 1) % capacity_;
    --size_;
    lock.unlock();
    cv_not_full_.notify_one();
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
  std::condition_variable cv_not_empty_;
  std::condition_variable cv_not_full_;
};

inline BackpressurePolicy ParseBackpressurePolicy(const std::string &policy) {
  if (policy == "block") {
    return BackpressurePolicy::kBlock;
  }
  if (policy == "drop_newest") {
    return BackpressurePolicy::kDropNewest;
  }
  return BackpressurePolicy::kDropOldest;
}

} // namespace aethersense
