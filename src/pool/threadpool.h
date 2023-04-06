// 线程池
#ifndef SERVER_POOL_THREADPOOL_H_
#define SERVER_POOL_THREADPOOL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>

template <typename T>
class Threadpool {
 public:
  Threadpool(size_t num_threads=8) {
    assert(num_threads > 0);
    for (size_t i = 0; i < num_threads; ++i) {
      // 线程处理函数
      auto worker = [pool = pool_]() {
        std::unique_lock<std::mutex> locker(pool->mtx, std::defer_lock);  // 互斥锁
        while (true) {
          locker.lock();  // 构造时不加锁，现在手动加锁
          if (!pool->tasks.empty()) {
            auto task = std::move(pool->tasks.front());  // 从请求队列中取出第一个
            pool->tasks.pop();  // 删除被取出的请求
            locker.unlock();
            task();  // 执行请求
            // locker.lock();
          } else if (pool->is_closed) {  // why?
            break;
          } else {
            pool->cond.wait(locker);  // 如果队列为空，在这里等待
          }
        }  // while
      };
      // 创建线程
      std::thread(worker).detach();  // 将新线程和调用线程分离，调用之后不再是joinable状态了
    }
  }
  Threadpool() = delete;
  Threadpool(Threadpool&&) = default;  // 移动构造

  ~Threadpool() {
    if (static_cast<bool>(pool_)) {  // why?
      {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->is_closed = true;
      }
      pool_->cond.notify_all();  // 唤醒所有等待的线程
    }
  }

  template<typename F>  // TODO
  void AddTask(F& task) {
    {
      std::lock_guard<std::mutex> locker(pool_->mtx);
      pool_->tasks.emplace(task);
    }
    pool_->cond.notify_one();  // 唤醒一个等待的进程
  }

 private:
  struct Pool {
    std::mutex mtx;
    std::condition_variable cond;
    bool is_closed;
    // function<return_type(args_type)>
    std::queue<std::function<void()>> tasks;  // 请求队列
  };
  std::shared_ptr<Pool> pool_;
};

#endif  // SERVER_POOL_THREADPOOL_H_