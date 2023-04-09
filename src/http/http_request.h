#ifndef WEBSERVER_HTTP_HTTP_CONNECT_H_
#define WEBSERVER_HTTP_HTTP_CONNECT_H_
// C header
#include <errno.h>
#include <mysql/mysql.h>
// C++ header
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <algorithm>
// Other header
// TODO: placeholder for log.h
#include "../pool/sql_connect_raii.h"
#include "../pool/sql_connect_pool.h"
#include "../buffer/buffer.h"

class HttpRequest {
 public:
  HttpRequest();
  ~HttpRequest() = default;
  
  bool Parse(Buffer* buff);

  inline bool IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
      return header_.at("Connection") == "keep-alive" && version_ == "1.1";
    }
    return false;
  }

  // 取值函数，获取path_的值
  inline const std::string& get_path() const {
    return path_;
  }
  inline std::string& get_path() {
    return path_;
  }

  // 取值函数，获取method_的值
  inline std::string get_method() const {
    return method_;
  }

  // 取值函数，获取version_的值
  inline std::string get_version() const {
    return version_;
  }

  // 取请求参数中的某个参数的对应值，const修饰的参数只能用at取值
  inline std::string GetPost(const std::string& key) const {
    assert(key != "");
    return (post_.count(key) == 1 ? post_.at(key) : "");
  }
  // 取请求参数中的某个参数的对应值
  inline std::string GetPost(const char* key) const {
    assert(key != nullptr);
    std::string str_key(key);
    return (post_.count(key) == 1 ? post_.at(str_key) : "");
  }

  // 解析位置状态
  enum ParseState {
    REQUEST_LINE,    // 请求行
    REQUEST_HEADER,  // 请求头
    REQUEST_CONTENT,    // 请求体
    REQUEST_FINISH,  // 请求结束
  };

  // HTTP请求处理结果
  enum HttpCode {
    NO_REQUEST,          // 请求不完整
    GET_REQUEST,         // 获得了完整的请求
    BAD_REQUEST,         // 请求报文有误
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,      // 服务器内部错误
    CLOSED_CONNECTION,
  };

 private:
  // 解析请求行
  bool ParseRequestLine(const std::string& line);
  // 解析请求头
  void ParseRequestHeader(const std::string& line);
  // 解析请求体
  void ParseRequestContent(const std::string& line);
  // 解析请求URL
  void ParsePath();
  // 解析POST请求，必须满足Content-Type = application/x-www-form-urlencoded
  void ParsePost();
  // 获取请求参数，以K-V形式放入post_中
  void ParseFormUrlEncoded();
  // 验证用户名，密码，登录/注册
  static bool UserVerify(const std::string& name, const std::string& pwd,
                         bool is_login);
  // convert %xy to integer
  inline int ConvertHexToInt(char x, char y) {
    int tmp_x = tolower(x) - 'a' + 10;
    int tmp_y = tolower(y) - 'a' + 10;
    return tmp_x * 16 + tmp_y;
  }
  // 解析状态
  ParseState state_;
  std::string method_;
  std::string path_;
  std::string version_;
  std::string content_;
  // request header: field=value
  std::unordered_map<std::string, std::string> header_;
  // request params: key=value
  std::unordered_map<std::string, std::string> post_;
  // 默认页面
  static const std::unordered_set<std::string> default_htmls;
  static const std::unordered_map<std::string, int> default_html_tags;
};

#endif  // WEBSERVER_HTTP_HTTP_CONNECT_H_