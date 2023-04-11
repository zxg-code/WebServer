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

  void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
  void MakeResponse(Buffer& buff);
  void UnmapFile();
  char* File();
  size_t FileLen() const;
  void ErrorContent(Buffer& buff, std::string message);
  int Code() const { return code_; }

 private:
  void AddStateLine(Buffer &buff);
  void AddHeader(Buffer &buff);
  void AddContent(Buffer &buff);

  void ErrorHtml();
  std::string GetFileType();

  int code_;
  bool is_keep_alive_;

  std::string path_;
  std::string src_dir_;
  
  char* mm_file_; 
  struct stat mm_file_stat_;

  static const std::unordered_map<std::string, std::string> suffix_type_;
  // http响应编码对应的解释
  static const std::unordered_map<int, std::string> code_status_;
  // http响应编码对应的页面
  static const std::unordered_map<int, std::string> code_path_;
};

#endif  // WEBSERVER_HTTP_HTTP_RESPONSE_H_