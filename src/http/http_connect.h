#ifndef WEBSERVER_HTTP_HTTP_CONNECT_H_
#define WEBSERVER_HTTP_HTTP_CONNECT_H_

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>

#include "../log/log.h"
#include "../pool/sql_connect_raii.h"
#include "../buffer/buffer.h"
#include "http_response.h"
#include "http_request.h"

class HttpConnect {
public:
  HttpConnect();
  ~HttpConnect();

  // 根据fd和socket地址初始化
  void Init(int sock_fd, const sockaddr_in& addr);
  // 读取消息并放入缓冲池，如失败则记录错误码
  ssize_t Read(int* save_errno);
  // 将数据发送到指定socket
  ssize_t Write(int* save_errno);
  // 关闭连接，取消文件映射，释放资源
  void Close();
  // 解析http请求数据
  bool Process();

  // 还需要写多少字节的数据
  inline int ToWriteBytes() { 
    return iov_[0].iov_len + iov_[1].iov_len; 
  }
  // 是否为长连接
  inline bool IsKeepAlive() const {
    return request_.IsKeepAlive();
  }
  // 获取socket对应的ip地址
  inline const char* GetIP() const { return inet_ntoa(addr_.sin_addr); }
  // 获取socket对应的端口
  inline int GetPort() const { return addr_.sin_port; }

  // 取值函数
  inline sockaddr_in get_addr() const { return addr_; }
  // 取值函数
  inline int get_fd() const { return fd_; }

  static bool is_ET;
  static const char* src_dir;
  static std::atomic<int> user_count;
    
private:
  int fd_;  // socket_fd
  struct  sockaddr_in addr_;
  bool is_close_;
  int iov_cnt_;
  struct iovec iov_[2];   
  Buffer* read_buff_; // 读缓冲区
  Buffer* write_buff_; // 写缓冲区
  HttpRequest request_;
  HttpResponse response_;
};

#endif  // WEBSERVER_HTTP_HTTP_CONNECT_H_