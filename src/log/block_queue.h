// Wrapping of deque to make it thread-safe.
// by zxg
//
#ifndef WEBSERVER_LOG_BLOCK_QUEUE_H_
#define WEBSERVER_LOG_BLOCK_QUEUE_H_

#include <sys/time.h>

#include <mutex>
#include <deque>
#include <condition_variable>

template<class T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t max_capacity) : capacity_(max_capacity) {
    assert(max_capacity > 0);
    is_close_ = false;
  }
  explicit BlockDeque() : capacity_(1000) {
    is_close_ = false;
  }

  ~BlockDeque() { Close(); }

  void Clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
  }

  bool Empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
  }

  bool Full() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
  }

  void Close() {
    {   
      std::lock_guard<std::mutex> locker(mtx_);
      deq_.clear();
      is_close_ = true;
    }
    cond_producer_.notify_all();
    cond_consumer_.notify_all();
  }

  size_t Size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
  }

  size_t Capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
  }

  T Front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
  }

  T Back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
  }

  void PushBack(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_) {
      cond_producer_.wait(locker);
    }
    deq_.push_back(item);
    cond_consumer_.notify_one();
  }

  void PushFront(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.size() >= capacity_) {
      cond_producer_.wait(locker);
    }
    deq_.push_front(item);
    cond_consumer_.notify_one();
  }

  bool Pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()){
      cond_consumer_.wait(locker);
      if (is_close_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
  }

  bool Pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while (deq_.empty()){
      if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout)) ==
          std::cv_status::timeout) return false;
      if (is_close_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

  void Flush() {
    cond_consumer_.notify_one();
  };

 private:
  bool is_close_;
  std::deque<T> deq_;
  size_t capacity_;  // 队列的最大容量

  std::mutex mtx_;
  std::condition_variable cond_consumer_;
  std::condition_variable cond_producer_;
};

#endif  // WEBSERVER_LOG_BLOCK_QUEUE_H_
