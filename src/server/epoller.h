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
  explicit Epoller(int max_event = 1024);

  ~Epoller();

  bool AddFd(int fd, uint32_t events);

  bool ModFd(int fd, uint32_t events);

  bool DelFd(int fd);

  int Wait(int timeout = -1);

  int GetEventFd(size_t i) const;

  uint32_t GetEvents(size_t i) const;
        
 private:
  int epoll_fd_;
  std::vector<struct epoll_event*> events_;    
};

#endif  // WEBSERVER_SERVER_EPOLLER_H_
