### server
#### epoller
#### epoll系统调用
<p>epoll把用户关心的文件描述符上的事件放在内核里的一个事件表中，而无须像select和poll那样重复传入。</p>
<p>因此epoll就需要一个额外的文件描述符来标识该事件表，大致流程如下：</p>

```cpp
#include <sys/epoll.h>
// size参数暂时不起作用，但必须大于0
// 返回文件描述符代表一个新的epoll实例，失败则返回-1
// 返回的文件描述符将用作其他所有epoll系统调用的第一个参数，即指定的内核事件表
int epoll_create(int size);  

// 注册感兴趣的事件
// This system call is used to add, modify, or remove entries in the interest list of the epoll(7) 
// instance referred to by the file descriptor epfd. 
// It requests that the operation op be performed for the target file descriptor: fd.
// params: 事件表fd; 操作; 目标fd; 事件
// return: 成功返回0，否则返回-1
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

// The epoll_wait() system call waits for events on the epoll(7) instance referred to by the file descriptor epfd.  The buffer pointed to by events is used to return information from the ready
// list about file descriptors in the interest list that have some events available.  Up to maxevents are returned by epoll_wait(). The maxevents argument must be greater than zero.
// The timeout argument specifies the number of milliseconds that epoll_wait() will block.  Time is measured against the CLOCK_MONOTONIC clock.
// return: 成功返回就绪的fd的个数，失败返回-1
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

