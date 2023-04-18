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
    