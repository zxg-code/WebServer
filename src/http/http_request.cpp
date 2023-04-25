#include "http_request.h"

using namespace std;

HttpRequest::HttpRequest() { Init(); }

void HttpRequest::Init() {
  method_ = "";
  path_ = "";
  version_ = "";
  content_ = "";
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();
}

const unordered_set<string> HttpRequest::default_htmls{
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

const unordered_map<string, int> HttpRequest::default_html_tags{
    {"/register.html", 0}, {"/login.html", 1},
};

bool HttpRequest::Parse(Buffer* buff) {
  const char CRLF[] = "\r\n";
  if (buff->ReadableBytes() <= 0) { return false; }
  while (buff->ReadableBytes() > 0 && state_ != REQUEST_FINISH) {
    // 每行以\r\n作为结束字符，查找\r\n就能将报文按行拆解
    const char* line_end = search(buff->Peek(),
                                  static_cast<const char*>(buff->BeginWrite()),
                                  CRLF, CRLF+2);
    // 读取一行
    std::string line(buff->Peek(), line_end);  // read line, construct string
    switch (state_) {
      case REQUEST_LINE:
        if (!ParseRequestLine(line)) { return false; }
        ParsePath();  // get html path
        break;
      case REQUEST_HEADER:
        ParseRequestHeader(line);
        if (buff->ReadableBytes() <= 2) { state_ = REQUEST_FINISH; }
        break;
      case REQUEST_CONTENT:
        ParseRequestContent(line);
        break;
      default:
        break;
    }  // switch
    if (line_end == buff->BeginWrite()) { 
      // 当方法体为POST的时候,此时会导致lineEnd已经走完了可读区域，
      // 提前退出而没有偏移readpos_
      if (method_ == "POST" && state_ == REQUEST_FINISH) 
        buff->RetrieveUntil(line_end);
      break; 
    }
    buff->RetrieveUntil(line_end + 2);  // 移动读指针，同时跳过CRLF
  }  // while
  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(),
            version_.c_str());
  return true;
}

bool HttpRequest::ParseRequestLine(const string& line) {
  regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");  // construct reg pattern
  smatch match_group;
  if (regex_match(line, match_group, pattern)) {
    method_ = match_group[1];   // eg: GET, POST
    path_ = match_group[2];     // URL
    version_ = match_group[3];  // version of http
    state_ = REQUEST_HEADER;    // next state
    return true;
  }
  LOG_ERROR("RequestLine Error");
  return false;
}

void HttpRequest::ParsePath() {
  if (path_ == "/") { path_ = "/index.html"; }  // home page
  else {
    for (const auto& item: default_htmls) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }  // for
  }
}

void HttpRequest::ParseRequestHeader(const string& line) {
  regex pattern("^([^:]*): ?(.*)$");  // construct reg pattern
  smatch match_group;
  if (regex_match(line, match_group, pattern)) {
    header_[match_group[1]] = match_group[2];  // field = content
  } else {
    state_ = REQUEST_CONTENT;  // next state，请求体解析结束才进入下一个状态
  }
}

void HttpRequest::ParseRequestContent(const string& line) {
  content_ = line;
  if (method_ == "POST" && 
      header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParsePost();
  }
  state_ = REQUEST_FINISH;
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::ParseFormUrlEncoded() {
  int n = content_.size();
  if (n == 0) return;
  string key;
  string value;
  int num = 0;       // do what?
  int L = 0, R = 0;  // left, right
  // Now, the format of content_ is similar to "key=value&key=value..."!
  // eg: "firstName=Mickey%26&lastName=Mouse+"
  for ( ; R < n; ++R) {
    char ch = content_[R];
    switch (ch) {
      case '=':
        key = content_.substr(L, R-L);  // start pos, length
        L = R + 1;
        break;
      case '+':
        content_[R] = ' ';  // 空格会被编码成'+'
        break;
      case '%':  // 特殊符号编码成16进制(%xy)，算三字节
        num = ConvertHexToInt(content_[R+1], content_[R+2]);
        content_[R+1] = num / 10 + '0';  // 有什么意义？
        content_[R+2] = num % 10 + '0';
        R += 2;
        break;
      case '&':  // 每对参数用'&'连接
        value = content_.substr(L, R-L);
        L = R + 1;
        post_[key] = value;
        LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
        break;
      default:
        break;
    }  // switch
  }  // for
  assert(L <= R);
  // 还有剩余，需要后处理
  if (post_.count(key) == 0 && L < R) {
    value = content_.substr(L, R-L);
    post_[key] = value;
  }
}

void HttpRequest::ParsePost() {
  ParseFormUrlEncoded();
  auto it = default_html_tags.find(path_);  // iterator
  if (it != default_html_tags.end()) {
    // auto [html, tag] = *it;  // [someone.html, tag]
    auto tag = it->second;
    LOG_DEBUG("Tag:%d", tag);
    if (tag == 0 || tag == 1) {
      bool is_login = (tag == 1);  // login or rigister
      if (UserVerify(post_["username"], post_["password"], is_login)) {
        path_ = "/welcome.html";
      } else {
        path_ = "/error.html";
      }
    } 
  }  // if
}

// TODO
bool HttpRequest::UserVerify(const string& name, const string& pwd, 
                             bool is_login) {
  if (name == "" || pwd == "") { return false; }
  LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  // construt sql pool
  MYSQL* sql;
  SqlConnectionRaii sql_pool(&sql, SqlConnectionPool::Instance()); 
  // auto sql = sql_pool.get_sql();  // get a sql connection
  assert(sql);
  bool flag = false;              // 返回值
  bool username_existed = false;  // 用户名是否已存在
  unsigned int num_fields = 0;    // 查询到的字段数
  char order[256] = {0};          // SQL语句
  MYSQL_FIELD* fields = nullptr;  // mysql字段结构体
  MYSQL_RES* res = nullptr;       // result set结构体

  // SQL语句，查询用户和密码根据用户名
  snprintf(order, sizeof(order),
           "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
           name.c_str());
  LOG_DEBUG("%s", order);
  if (mysql_query(sql, order) != 0) {  // 0 for success, nonzero if error
    mysql_free_result(res);  // 释放结果集的内存
    return false;
  }
  res = mysql_store_result(sql);       // 保存查询到的结果集
  num_fields = mysql_num_fields(res);  // columns in a result set
  fields = mysql_fetch_fields(res);    // 获取结果集的字段

  while(MYSQL_ROW row = mysql_fetch_row(res)) {
    LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
    string password(row[1]);
    // 登录
    if (is_login) {  // 登录行为
        if (pwd == password) { flag = true; }  // 验证密码
        else {
          flag = false;
          LOG_DEBUG("pwd error!");
        }
    } else {  // 用户不是登录行为，却查询到了用户名和密码，说明用户名被占用
      username_existed = true;  // 用户名已经存在
      flag = false;
      LOG_DEBUG("user used!");
    }
  }

  // 如果是注册行为且用户名未被占用
  if (!is_login && !username_existed) {
    LOG_DEBUG("regirster!");
    bzero(order, 256);
    // 插入命令
    snprintf(order, 256,
              "INSERT INTO user(username, password) VALUES('%s','%s')",
              name.c_str(), pwd.c_str());
    LOG_DEBUG( "%s", order);
    if(mysql_query(sql, order) == 0) {  // 插入一条记录，命令执行成功
        flag = true;
    }
  }
  LOG_DEBUG( "UserVerify success!!");
  mysql_free_result(res);  // 释放内存
  return flag;
}