#ifndef WEBSERVER_HTTP_HTTP_RESPONSE_H_
#define WEBSERVER_HTTP_HTTP_RESPONSE_H_

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unordered_map>

#include "../buffer/buffer.h"
// TODO: LOG.H

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void Init(const std::string& srcDir, std::string& path,
            bool isKeepAlive = false, int code = -1);
  // 组建报文响应请求
  void MakeResponse(Buffer* buff);
  // 结束内存映射，释放资源
  void UnmapFile();
  // 构建错误提示消息，写入缓冲池
  void ErrorContent(Buffer* buff, std::string message);
  // 获取文件的长度，用于mmap
  inline size_t FileLen() const { return mm_file_stat_.st_size; }
  // 取值函数，获取code_
  inline int get_code() const { return code_; }
  // 取值函数，获取mm_file_
  inline char* get_mm_file() { return mm_file_; }

 private:
  // 将响应消息中的状态行写入到缓冲池中
  void AddStateLine(Buffer* buff);
  // 将响应消息中的头部字段写入缓冲池中
  void AddHeader(Buffer* buff);
  // 将响应消息中的消息体写入缓冲池中
  void AddContent(Buffer* buff);
  // 若返回码为400，403，404其中之一，则将对应的文件路径与信息读取到对应的变量中
  void ErrorHtml();
  // 获取文件对应的返回类型
  std::string GetFileType();

  int code_;             // 状态码
  bool is_keep_alive_;   // 是否长连接
  std::string path_;     // 响应文件路径
  std::string src_dir_;  // 文件目录
  char* mm_file_;             // mmap映射文件所在地址
  struct stat mm_file_stat_;  // 路径为 src_dir_+path_ 的文件的状态信息
  // 不同文件后缀对应的返回类型
  static const std::unordered_map<std::string, std::string> suffix_type_;
  // http响应编码对应的解释
  static const std::unordered_map<int, std::string> code_status_;
  // http错误编码对应的页面
  static const std::unordered_map<int, std::string> code_path_;
};

#endif  // WEBSERVER_HTTP_HTTP_RESPONSE_H_