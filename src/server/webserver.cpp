// by zxg
//
#include "webserver.h"

WebServer::WebServer(int port, int trig_mode, int timeout, bool opt_linger,
                     int sql_port, const char* sql_user, const char* sql_pwd, 
                     const char* db_name, int num_conn_pool, int num_threads,
                     bool open_log, int log_level, int log_que_size)
    : port_(port),
      open_linger_(opt_linger),
      timeout_(timeout),
      is_close_(false),
      timer_(new HeapTimer()),  // 智能指针，不用自己释放
      threadpool_(new Threadpool(num_threads)),
      epoller_(new Epoller()) {
  src_dir_ = getcwd(nullptr, 256);  // 资源目录
  assert(src_dir_);
  strncat(src_dir_, "/resources/", 16);
  // 初始化http连接类的静态变量
  HttpConnect::user_count = 0;
  HttpConnect::src_dir = src_dir_;
  // 获取数据库连接池实例
  SqlConnectionPool::Instance()->Init("localhost", sql_port, sql_user, sql_pwd,
                                      db_name, num_conn_pool);
  InitEventMode(trig_mode);  // 确定事件工作模式
  if (!InitSocket()) is_close_ = true;
  // 开启日志
  if (open_log) {
    Log::Instance()->Init(log_level, "./log", ".log", log_que_size);
    if (is_close_) {
      LOG_ERROR("========== Server init error!==========");
    } else {
      LOG_INFO("========== Server init ==========");
      LOG_INFO("Port:%d, OpenLinger: %s", port_, opt_linger? "true":"false");
      LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", 
               (listen_event_ & EPOLLET ? "ET": "LT"),
               (conn_event_ & EPOLLET ? "ET": "LT"));
      LOG_INFO("LogSys level: %d", log_level);
      LOG_INFO("srcDir: %s", HttpConnect::src_dir);
      LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", num_conn_pool,
               num_threads);
    }  // else
  }  // if
}

WebServer::~WebServer() {
  close(listen_fd_);
  is_close_ = true;
  free(src_dir_);
  SqlConnectionPool::Instance()->CloseSqlConnPool();
}

void WebServer::InitEventMode(int trig_mode) {
  listen_event_ = EPOLLRDHUP;  // 初始化epoll事件为：对端关闭连接
  // EPOLLONESHOT: 只处理一次，然后从事件表中删除
  conn_event_ = EPOLLONESHOT | EPOLLRDHUP;  // 初始化epoll事件
  // 设置epoll模式，设为ET(边沿触发)
  switch (trig_mode) {
    case 0:
      break;
    case 1:
      conn_event_ |= EPOLLET;
      break;
    case 2:
      listen_event_ |= EPOLLET;
      break;
    case 3:  // dont need
      listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
    default:
      listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
  }
  // 初始化http连接类的静态变量
  HttpConnect::is_ET = (conn_event_ & EPOLLET);
}

bool WebServer::InitSocket() {
  int ret;
  struct sockaddr_in addr;
  // 端口检查
  if (port_ > 65535 || port_ < 1024) {
    LOG_ERROR("Port:%d error!",  port_);
    return false;
  }
  // 设置监听地址、端口等
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);  // host byte order to network long
  addr.sin_port = htons(port_);
  // socket option: SO_LINGER
  struct linger optLinger = { 0 };  // off is default setting
  if (open_linger_) {
    // 设置linger,
    optLinger.l_onoff = 1;   // option is disabled if the value is 0
    optLinger.l_linger = 1;  // linger time on close (units: seconds)
  }
  // create server socket and set option
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    LOG_ERROR("Create socket error!", port_);
    return false;
  }
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &optLinger, 
                   sizeof(optLinger));
  if (ret < 0) {
    close(listen_fd_);
    LOG_ERROR("Init linger error!", port_);
    return false;
  }
  // 开启端口复用选项
  int reuse = 1;
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse,
                   sizeof(int));
  if (ret == -1) {
    LOG_ERROR("set socket setsockopt error !");
    close(listen_fd_);
    return false;
  }
  // 绑定
  ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERROR("Bind Port:%d error!", port_);
    close(listen_fd_);
    return false;
  }
  // 监听，设定队列长度
  ret = listen(listen_fd_, 6);
  if (ret < 0) {
    LOG_ERROR("Listen port:%d error!", port_);
    close(listen_fd_);
    return false;
  }
  // 加入监听事件描述符集
  ret = epoller_->AddFd(listen_fd_,  listen_event_ | EPOLLIN);
  if (ret == 0) {
    LOG_ERROR("Add listen error!");
    close(listen_fd_);
    return false;
  }
  // 设置监听事件为非阻塞
  SetFdNonblock(listen_fd_);
  LOG_INFO("Server port:%d", port_);
  return true;
}

int WebServer::SetFdNonblock(int fd) {
  assert(fd > 0);
  // fcntl params: fd, cmd, args
  // F_GETFD: get fd flags
  // F_SETFL: set the file status flags
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::SendError(int fd, const char*info) {
  assert(fd > 0);
  int ret = send(fd, info, strlen(info), 0);
  if (ret < 0) LOG_WARN("send error to client[%d] error!", fd);
  close(fd);  // 为什么不调用CloseConnect
}

void WebServer::CloseConnect(HttpConnect* client) {
  assert(client);
  LOG_INFO("Client[%d] quit!", client->get_fd());
  epoller_->DelFd(client->get_fd());
  client->Close();
}

void WebServer::Start() {
  int time_ms = -1;  // epoll wait timeout == -1 无事件将阻塞
  if (!is_close_) LOG_INFO("========== Server start ==========");
  // 启动服务
  while (!is_close_) {
    // 如果设置了超时时间，需要处理超时事件
    if (timeout_ > 0) time_ms = timer_->GetNextTick();
    int num_events = epoller_->Wait(time_ms);  // 就绪事件数
    // 处理事件
    for (int i = 0; i < num_events; i++) {
      int fd = epoller_->GetEventFd(i);
      uint32_t events = epoller_->GetEvents(i);
      // 分情况处理
      if (fd == listen_fd_) {
        DealConnect();
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  // 关闭或挂起
        assert(users_.count(fd) > 0);
        CloseConnect(&users_[fd]);
      } else if (events & EPOLLIN) {   // 读事件
        assert(users_.count(fd) > 0);
        DealRead(&users_[fd]);
      } else if (events & EPOLLOUT) {  // 写事件
        assert(users_.count(fd) > 0);
        DealWrite(&users_[fd]);
      } else LOG_ERROR("Unexpected event");  // 错误
    }  // for
  }  // while
}

void WebServer::AddClient(int conn_fd, sockaddr_in cli_addr) {
  assert(conn_fd > 0);
  users_[conn_fd].Init(conn_fd, cli_addr);  // 创建并初始化一个httpconnect对象
  // 需要增加一个定时器，超时则触发关闭连接函数
  if (timeout_ > 0) {
    timer_->AddTimer(conn_fd, timeout_, std::bind(&WebServer::CloseConnect,
                                                  this, &users_[conn_fd]));
  }
  // 添加epoll监听事件
  epoller_->AddFd(conn_fd, EPOLLIN | conn_event_);
  SetFdNonblock(conn_fd);  // 连接设为非阻塞
  LOG_INFO("Client[%d] in!", users_[conn_fd].get_fd());
}

void WebServer::DealConnect() {
  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  do {
    int fd = accept(listen_fd_, (struct sockaddr *)&cli_addr, &cli_len);
    if (fd < 0) return;  // or <= ?
    else if (HttpConnect::user_count >= MAX_FD_) {  // too many clients
      SendError(fd, "Server busy!");
      LOG_WARN("Clients is full!");
      return;
    }
    AddClient(fd, cli_addr);  // add timer or epoll events
  } while (listen_event_ & EPOLLET);
}

void WebServer::DealRead(HttpConnect* client) {
  assert(client);
  ExtentTime(client);  // 调整连接的过期时间
  // 向线程池任务队列中增加一个读任务
  threadpool_->AddTask(std::bind(&WebServer::OnRead, this, client));
}

void WebServer::DealWrite(HttpConnect* client) {
  assert(client);
  ExtentTime(client);
  // 向线程池任务队列中增加一个写任务
  threadpool_->AddTask(std::bind(&WebServer::OnWrite, this, client));
}

void WebServer::ExtentTime(HttpConnect* client) {
  assert(client);
  if (timeout_ > 0) timer_->Adjust(client->get_fd(), timeout_);
}

void WebServer::OnRead(HttpConnect* client) {
  assert(client);
  int len = -1;  // 读取的长度，字节数
  int readErrno = 0;
  len = client->Read(&readErrno);
  if (len <= 0 && readErrno != EAGAIN) {
    CloseConnect(client);  // 发生错误
    return;
  }
  OnProcess(client);
}

void WebServer::OnProcess(HttpConnect* client) {
  if (client->Process()) {  // 没有可读数据会返回false
    // 如果请求解析成功则将对应的epoll事件改为写事件
    epoller_->ModFd(client->get_fd(), conn_event_ | EPOLLOUT);
  } else {
    // 否则相反
    epoller_->ModFd(client->get_fd(), conn_event_ | EPOLLIN);
  }
}

void WebServer::OnWrite(HttpConnect* client) {
  assert(client);
  int len = -1;  // 写入的长度，字节数
  int writeErrno = 0;
  len = client->Write(&writeErrno);
  // 如果没有数据需要写了
  if (client->ToWriteBytes() == 0) {
    // 传输完成,如果客户端设置了长连接，那么调用OnProcess函数，因为此时的client->process()
    // 会返回false，所以该连接会重新注册epoll的EPOLLIN事件
    if (client->IsKeepAlive()) {
      OnProcess(client);
      return;
    }
  } else if (len < 0) {
    // 若返回值小于0，且信号为EAGAIN说明数据还没有发送完
    // 重新在EPOLL上注册该连接的EPOLLOUT事件*/
    if (writeErrno == EAGAIN) {
      epoller_->ModFd(client->get_fd(), conn_event_ | EPOLLOUT);
      return;
    }
  }
  CloseConnect(client);  // 否则关闭连接
}