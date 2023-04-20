// socket server
// by zxg
//
#ifndef WEBSERVER_SERVER_WEBSERVER_H_
#define WEBSERVER_SERVER_WEBSERVER_H_

#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unordered_map>
#include <cstring>

#include "../pool/threadpool.h"
#include "../pool/sql_connect_raii.h"
#include "../pool/sql_connect_pool.h"
#include "../http/http_connect.h"
#include "../timer/heaptimer.h"
#include "../log/log.h"
#include "epoller.h"

class WebServer {
 public:
  // params: 
  WebServer(int port, int trig_mode, int timeout, bool opt_linger,
            int sql_port, const char* sql_user, const char* sql_pwd, 
            const char* db_name, int num_conn_pool, int num_threads,
            bool open_log, int log_level, int log_que_size);
  ~WebServer();
  // 启动服务器
  void Start();

 private:
  // 创建服务端监听套接字
  bool InitSocket();
  // 初始化事件工作模式
  void InitEventMode(int trig_mode);
  // 根据客户的fd初始化httpconnect，添加对应的计时器和epoll监听事件
  void AddClient(int conn_fd, sockaddr_in cli_addr);
  // 处理连接事件
  void DealConnect();
  // 处理写事件
  void DealWrite(HttpConnect* client);
  // 处理读事件
  void DealRead(HttpConnect* client);
  // 向客户端发送给错误消息并关闭连接
  void SendError(int fd, const char* info);
  // 延长当前连接的过期时间
  void ExtentTime(HttpConnect* client);
  // 删除epoll监听事件，关闭连接
  void CloseConnect(HttpConnect* client);
  // 读取一条连接上的数据
  void OnRead(HttpConnect* client);
  // 向一条连接上写数据
  void OnWrite(HttpConnect* client);
  // 处理数据
  void OnProcess(HttpConnect* client);
  // 将描述符fd设为非阻塞状态
  static int SetFdNonblock(int fd);

  static const int MAX_FD_ = 65536;  // 最大客户数量
  int port_;          // 服务器端口
  bool open_linger_;  // socket选项SO_LINGER是否开启，用来处理在close()时残留的数据，丢弃或继续发送
  int timeout_;       // 超时时间，毫秒MS
  bool is_close_;     // 初始化套接字是否成功，成功则表示服务开启，为false
  int listen_fd_;     // 监听的socket
  char* src_dir_;     // 资源文件目录
  
  uint32_t listen_event_;  // 监听的socket上发生的事件
  uint32_t conn_event_;    // 一个连接上发生的事件
  
  std::unique_ptr<HeapTimer> timer_;
  std::unique_ptr<Threadpool> threadpool_;
  std::unique_ptr<Epoller> epoller_;
  // fd和客户连接之间的映射，方便快速找到一个连接
  std::unordered_map<int, HttpConnect> users_;
};


#endif  // WEBSERVER_SERVER_WEBSERVER_H_