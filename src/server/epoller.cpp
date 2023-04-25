// by zxg
//
#include "epoller.h"

Epoller::Epoller()
    : epoll_fd_(epoll_create(512)), events_(1024) {
  assert(epoll_fd_ >= 0);
}

Epoller::Epoller(int max_event)
    : epoll_fd_(epoll_create(512)), events_(max_event) {
  assert(epoll_fd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
  close(epoll_fd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
  if (fd < 0) return false;
  epoll_event ev = {0};
  ev.data.fd = fd;     // 用户数据，包含一些传递的参数
  ev.events = events;  // epoll注册的事件
  // params: op: add entry to the interest list
  return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) {
  if (fd < 0) return false;
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  // params: op: modify setting associated with fd in the interest list
  return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) {
  if (fd < 0) return false;
  epoll_event ev = {0};
  // params: op: remove target fd from the interest list
  return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeout) {
  // epoll_wait需要接受一个数组保存事件
  return epoll_wait(epoll_fd_, &(*events_.begin()), 
                    static_cast<int>(events_.size()), timeout);
}

int Epoller::GetEventFd(size_t i) const {
  assert(i < events_.size() && i >= 0);
  return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
  assert(i < events_.size() && i >= 0);
  return events_[i].events;
}