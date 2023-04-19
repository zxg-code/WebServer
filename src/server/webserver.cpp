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
  listen_event_ = EPOLLRDHUP;  // 初始化epoll事件为：“连接关闭”
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