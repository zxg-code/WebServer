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
  BlockDeque() : capacity_(1000) {}

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


// template<class T>
// BlockDeque<T>::BlockDeque(size_t MaxCapacity) : capacity_(max_capacity) {
//     assert(MaxCapacity > 0);
//     isClose_ = false;
// }

// template<class T>
// BlockDeque<T>::~BlockDeque() {
//     Close();
// };

// template<class T>
// void BlockDeque<T>::Close() {
//     {   
//         std::lock_guard<std::mutex> locker(mtx_);
//         deq_.clear();
//         isClose_ = true;
//     }
//     condProducer_.notify_all();
//     condConsumer_.notify_all();
// };

// template<class T>
// void BlockDeque<T>::flush() {
//     condConsumer_.notify_one();
// };

// template<class T>
// void BlockDeque<T>::clear() {
//     std::lock_guard<std::mutex> locker(mtx_);
//     deq_.clear();
// }

// template<class T>
// T BlockDeque<T>::front() {
//     std::lock_guard<std::mutex> locker(mtx_);
//     return deq_.front();
// }

// template<class T>
// T BlockDeque<T>::back() {
//     std::lock_guard<std::mutex> locker(mtx_);
//     return deq_.back();
// }

// template<class T>
// size_t BlockDeque<T>::size() {
//     std::lock_guard<std::mutex> locker(mtx_);
//     return deq_.size();
// }

// template<class T>
// size_t BlockDeque<T>::capacity() {
//     std::lock_guard<std::mutex> locker(mtx_);
//     return capacity_;
// }

// template<class T>
// void BlockDeque<T>::push_back(const T &item) {
//     std::unique_lock<std::mutex> locker(mtx_);
//     while(deq_.size() >= capacity_) {
//         condProducer_.wait(locker);
//     }
//     deq_.push_back(item);
//     condConsumer_.notify_one();
// }

// template<class T>
// void BlockDeque<T>::push_front(const T &item) {
//     std::unique_lock<std::mutex> locker(mtx_);
//     while(deq_.size() >= capacity_) {
//         condProducer_.wait(locker);
//     }
//     deq_.push_front(item);
//     condConsumer_.notify_one();
// }

// template<class T>
// bool BlockDeque<T>::empty() {
//     std::lock_guard<std::mutex> locker(mtx_);
//     return deq_.empty();
// }

// template<class T>
// bool BlockDeque<T>::full(){
//     std::lock_guard<std::mutex> locker(mtx_);
//     return deq_.size() >= capacity_;
// }

// template<class T>
// bool BlockDeque<T>::pop(T &item) {
//     std::unique_lock<std::mutex> locker(mtx_);
//     while(deq_.empty()){
//         condConsumer_.wait(locker);
//         if(isClose_){
//             return false;
//         }
//     }
//     item = deq_.front();
//     deq_.pop_front();
//     condProducer_.notify_one();
//     return true;
// }

// template<class T>
// bool BlockDeque<T>::pop(T &item, int timeout) {
//     std::unique_lock<std::mutex> locker(mtx_);
//     while(deq_.empty()){
//         if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
//                 == std::cv_status::timeout){
//             return false;
//         }
//         if(isClose_){
//             return false;
//         }
//     }
//     item = deq_.front();
//     deq_.pop_front();
//     condProducer_.notify_one();
//     return true;
// }

#endif  // WEBSERVER_LOG_BLOCK_QUEUE_H_
