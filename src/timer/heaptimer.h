// 小根堆
#ifndef WEBSERVER_TIMER_HEAPTIMER_H_
#define WEBSERVER_TIMER_HEAPTIMER_H_

#include <time.h>
#include <arpa/inet.h>
#include <assert.h>

#include <queue>
#include <unordered_map>
#include <algorithm>
#include <functional> 
#include <chrono>  // cpp time library

// TODO: #include "../log/log.h"

using TimeoutCallBack = std::function<void()>;  // 
using Clock = std::chrono::high_resolution_clock;  
using MS = std::chrono::microseconds;  // 时间段
using TimeStamp = Clock::time_point;   // 时间点

// 堆节点，以超时时间为依据
struct TimerNode {
  int id;
  TimeStamp expires;   // 超时时间
  TimeoutCallBack cb;  // 回调函数
  // 重载比较运算符
  bool operator<(const TimerNode& t) {
    return expires < t.expires;
  }
  TimerNode(int id, TimeStamp expires, TimeoutCallBack cb)
    : id(id), expires(expires), cb(cb) {}
                                                          
};

class HeapTimer {
 public:
  HeapTimer() { heap_.reserve(64); }

  ~HeapTimer() { Clear(); }
  // 调整指定描述符对应的节点的过期时间为当前时间+timeout
  void Adjust(int id, int timeout);
  // 为指定文件描述符添加或更新一个定时器
  void AddTimer(int id, int timeout, const TimeoutCallBack& cb);

  void DoWork(int id);
  // 清空整个堆和映射
  void Clear();
  // 删除堆中的所有超时节点
  void Tick();
  // 删除堆顶节点
  void Pop();
  // 删除所有超时节点，返回堆顶节点的剩余时间
  int GetNextTick();

 private:
  // 删除指定下标的timer
  void DelTimer(size_t idx);
  // 堆调整函数
  // params: i: 在数组中下标，表示从该位置开始向上调整
  void SiftUp(size_t idx, size_t n);
  // 堆调整函数
  // params: idx: 在数组中下标，表示从该位置开始向上调整
  //         n: 堆大小
  bool SiftDown(size_t idx, size_t n);
  void SwapNode(size_t i, size_t j);

  std::vector<TimerNode> heap_;  // 小根堆
  // 文件描述符和所在堆下标的映射
  std::unordered_map<int, size_t> ref_;  // 映射关系
};

#endif  // WEBSERVER_TIMER_HEAPTIMER_H_