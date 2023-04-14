#include "heaptimer.h"

void HeapTimer::SwapNode(size_t i, size_t j) {
  assert(i >= 0 && i < heap_.size());
  assert(j >= 0 && j < heap_.size());
  std::swap(heap_[i], heap_[j]);
  // 同时更新文件描述符和堆下标映射
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
} 

void HeapTimer::SiftUp(size_t idx, size_t n) {
  assert(idx >= 0 && idx < heap_.size());
  size_t parent = (idx - 1) / 2;  // 父节点
  while (parent >= 0) {
    if (heap_[parent] < heap_[idx]) break;  // 加个等号？
    SwapNode(idx, parent);
    // 向上移动
    idx = parent;
    parent = (idx - 1) / 2;
  }
}

bool HeapTimer::SiftDown(size_t idx, size_t n) {
  assert(idx >= 0 && idx < heap_.size());
  assert(n >= 0 && n <= heap_.size());
  size_t i = idx; 
  size_t left = i * 2 + 1;  // 左孩子，j+1表示右孩子
  while (left < n) {
    // 选出左右孩子中最小的
    if (left + 1 < n && heap_[left + 1] < heap_[left]) left++;
    if (heap_[i] < heap_[left]) break;  // 如果父节点值已经比子节点小了
    SwapNode(i, left);
    // 向下移动
    i = left;
    left = i * 2 + 1;  
  }
  return i > idx;  // 表明是否发生过交换
}

void HeapTimer::AddTimer(int id, int timeout, const TimeoutCallBack& cb) {
  assert(id >= 0);
  size_t i;
  if (ref_.count(id) == 0) {
    // 新节点：堆尾插入，向上调整堆
    i = heap_.size();
    ref_[id] = i;
    // heap_.push_back({id, Clock::now() + MS(timeout), cb});
    heap_.emplace_back(id, Clock::now() + MS(timeout), cb);  // 构造，不是类型转换
    SiftUp(i, heap_.size());
  } else {
    // 已有结点：调整堆
    i = ref_[id];
    // 更新超时时间
    heap_[i].expires = Clock::now() + MS(timeout);
    heap_[i].cb = cb;
    // 如果修改节点值后没有发生向下调整，那就向上调整
    if (!SiftDown(i, heap_.size())) SiftUp(i, heap_.size());
  }
}

void HeapTimer::DoWork(int id) {
  // 删除指定id结点，并触发回调函数
  if (heap_.empty() || ref_.count(id) == 0) return;  // 判断条件应该多了
  size_t i = ref_[id];
  TimerNode node = heap_[i];
  node.cb();
  DelTimer(i);
}

void HeapTimer::DelTimer(size_t idx) {
  // 删除指定位置的结点
  assert(!heap_.empty() && idx >= 0 && idx < heap_.size());
  // 将要删除的结点换到堆尾，然后调整它之前的节点
  size_t i = idx;
  size_t n = heap_.size() - 1;
  assert(i <= n);
  if (i < n) {
    SwapNode(i, n);
    if (!SiftDown(i, n)) SiftUp(i, n);
  }
  // 堆尾元素删除
  ref_.erase(heap_.back().id);
  heap_.pop_back();
}

void HeapTimer::Adjust(int id, int timeout) {
  // 调整指定id的结点
  assert(!heap_.empty() && ref_.count(id) > 0);
  heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
  SiftDown(ref_[id], heap_.size());
}

void HeapTimer::Tick() {
  // 查看堆顶，如果超时则删除
  if (heap_.empty()) return;
  while (!heap_.empty()) {
    TimerNode node = heap_.front();
    // 未超时
    if (std::chrono::duration_cast<MS>(node.expires - Clock::now())
        .count() > 0) break; 
    node.cb();
    Pop();
  }
}

// 删除堆顶节点
void HeapTimer::Pop() {
  assert(!heap_.empty());
  DelTimer(0);
}

void HeapTimer::Clear() {
  ref_.clear();
  heap_.clear();
}

int HeapTimer::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap_.empty()) {
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now())
          .count();
    if (res < 0) res = 0; 
  }
  return res;
}
