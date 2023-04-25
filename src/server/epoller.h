// Some wrapper for the epoll system call functions.
// by zxg
//
#ifndef WEBSERVER_SERVER_EPOLLER_H_
#define WEBSERVER_SERVER_EPOLLER_H_

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <errno.h>

#include <vector>

class Epoller {
 public:
  explicit Epoller();  // default max_event: 1024
  explicit Epoller(int max_event);
  ~Epoller();

  // 添加监听事件
  bool AddFd(int fd, uint32_t events);
  // 改变被监听事件中的某个fd对应的设置，新设置在events中
  bool ModFd(int fd, uint32_t events);
  // 删除指定的事件
  bool DelFd(int fd);
  // 在一段超时时间内等待事件（milliseconds）
  int Wait(int timeout = -1);  // -1 means block
  // 获取对应下标的epoll事件所从属的目标fd
  int GetEventFd(size_t i) const;
  // 获取对应下标的epoll事件
  uint32_t GetEvents(size_t i) const;
        
 private:
  int epoll_fd_;  // 指定的内核事件表
  std::vector<struct epoll_event> events_;  // 监听的事件集
};

#endif  // WEBSERVER_SERVER_EPOLLER_H_
