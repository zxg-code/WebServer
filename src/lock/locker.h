// 线程同步机制类的封装，包括以下三种
// 信号量: SemaphoreWrapper
// 互斥量：LockerWrapper
// 条件变量：

#ifndef SERVER_LOCK_LOCKER_H_
#define SERVER_LOCK_LOCKER_H_

#include <pthread.h>
#include <semaphore.h>

#include <exception>

// 对信号量的封装
class SemaphoreWrapper {
 public:
  SemaphoreWrapper() {
    // 初始化一个未命名的信号量，设定初值
    // params: sem指针, 是否进程间共享, 初值
    // shared: 0表示一个进程的线程间共享，其他表示进程间共享
    if (sem_init(&m_sem_, 0, 0) != 0) throw std::exception();  // ret: 0 or -1
  }

  SemaphoreWrapper(int value) {
    if (sem_init(&m_sem_, 0, value) != 0) throw std::exception();
  }

  ~SemaphoreWrapper() {
    // 销毁一个信号量，只有被sem_init的信号量才应该被sem_destory
    sem_destroy(&m_sem_);
  }

  bool Wait() {
    // 信号量值减1，如果值为0则阻塞，直到不为0
    return sem_wait(&m_sem_) == 0;  // ret: 0 or -1
  }

  bool Post() {
    // 信号量值加1，如果值大于0了，其他正在调用sem-wait等待的线程或进程将被唤醒
    return sem_post(&m_sem_) == 0;  // ret: 0 or -1
  }

 private:
  sem_t m_sem_;
};

// 封装互斥量
class LockerWrapper {
 public:
  LockerWrapper() {
    // 初始化互斥量
    // params: 互斥量，mutex属性（如果为NULL，则用默认属性）
    // ret: 0 or error condition
    if (pthread_mutex_init(&m_mutex_, NULL) != 0) throw std::exception();
  }

  ~LockerWrapper() {
    // 销毁互斥锁
    pthread_mutex_destroy(&m_mutex_);
  }

  bool Lock() {
    // 以原子方式给一个互斥锁加锁，如果已经被锁了，则阻塞直到占有者解锁
    // ret: 0 or error number
    return pthread_mutex_lock(&m_mutex_) == 0;
  }

  bool Unlock() {
    // 以原子方式给一个互斥量解锁
    // ret: 0 or error number
    return pthread_mutex_unlock(&m_mutex_) == 0;
  }
  
  // 取互斥量的值
  pthread_mutex_t* get_m_mutex() {
    return &m_mutex_;
  } 

 private:
  pthread_mutex_t m_mutex_;
};

// 封装条件变量
class ConditionWrapper {
 public:
  ConditionWrapper() {
    // 初始化条件变量
    // params: 条件变量，cond属性
    // ret: 0 or errno
    if (pthread_cond_init(&m_cond_, NULL) != 0) throw std::exception();
  }

  ~ConditionWrapper() {
    pthread_cond_destroy(&m_cond_);
  }

  bool Wait(pthread_mutex_t *m_mutex) {
    // params: cond, mutex
    // mutex参数是保护条件变量的互斥量，在执行函数前，保证mutex上锁了
    // ret: 0 or errno
    return pthread_cond_wait(&m_cond_, m_mutex) == 0;
  }

  bool TimeWait(pthread_mutex_t* m_mutex, struct timespec timeout) {  // 指针？
    // timout 超时时间
    return pthread_cond_timedwait(&m_cond_, m_mutex, &timeout) == 0;
  }

  bool Signal() {
    // 唤醒一个等待目标条件变量的线程，取决于线程优先级和调度策略
    return pthread_cond_signal(&m_cond_) == 0;
  }

  bool Broadcast() {
    // 以广播的方式唤醒所有等待目标条件变量的线程
    return pthread_cond_broadcast(&m_cond_) == 0;
  }

 private:
  pthread_cond_t m_cond_;  // 条件变量
};

#endif  // SERVER_LOCK_LOCKER_H_