#include "http_request.h"

using namespace std;

HttpRequest::HttpRequest()
    : method_(""), 
      path_(""), 
      version_(""), 
      content_(""), 
      state_(REQUEST_LINE) {
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
    const char* line_end = search(buff->Peek(), buff->BeginWrite(),
                                  CRLF, CRLF+2);
    // 读取一行
    std::string line(buff->Peek(), line_end);  // read line, construct string
    switch (state_) {  // TODO:
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
    if (line_end == buff->BeginWrite()) { break; }  // 不用移动读指针吗？
    buff->RetrieveUntil(line_end + 2);  // 移动读指针，同时跳过CRLF
  }  // while
  // TODO: log debug
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
  // TODO: log error
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
    state_ = REQUEST_CONTENT;  // next state
  }
}

void HttpRequest::ParseRequestContent(const string& line) {
  content_ = line;
  if (method_ == "POST" && 
      header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParsePost();
  }
  state_ = REQUEST_FINISH;
  // TODO: log debug
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
        // TODO: log debug
        break;
      default:
        break;
    }  // switch
  }  // for
}

void HttpRequest::ParsePost() {
  ParseFormUrlEncoded();
  auto it = default_html_tags.find(path_);  // iterator
  if (it != default_html_tags.end()) {
    auto [html, tag] = *it;  // [someone.html, tag]
    // TODO: log debug
    if (tag == 0 || tag == 1) {
      bool is_login = (tag == 1);
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
  // TODO: log info
  SqlConnectionRaii sql_pool(SqlConnectionPool::Instance());  // construt sql pool
  auto sql = sql_pool.get_sql();  // get a sql connection
  assert(sql);
  bool flag = false;              // 返回值
  bool username_existed = false;  // 用户名已存在
  unsigned int num_fields = 0;
  char order[256] = {0};
  MYSQL_FIELD* fields = nullptr;  // mysql字段结构体
  MYSQL_RES* res = nullptr;       // result set结构体

  // SQL语句，查询用户和密码
  snprintf(order, sizeof(order),
           "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
           name.c_str());
  // TODO: log debug
  if (mysql_query(sql, order) != 0) {  // 0 for success, nonzero if error
    mysql_free_result(res);  // 释放结果集的内存
    return false;
  }
  res = mysql_store_result(sql);       // 保存查询到的结果集
  num_fields = mysql_num_fields(res);  // columns in a result set
  fields = mysql_fetch_fields(res);    // 获取结果集的字段

  while(MYSQL_ROW row = mysql_fetch_row(res)) {
    // TODO: LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
    string password(row[1]);
    // 登录
    if (is_login) {  // 登录行为
        if (pwd == password) { flag = true; }  // 验证密码
        // else: TODO: LOG_DEBUG("pwd error!");
    } else {  // 用户不是登录行为，却查询到了用户名和密码，说明用户名被占用
      username_existed = true;  // 用户名已经存在
    }
  }

  // 如果是注册行为且用户名未被占用
  if (!is_login && !username_existed) {
    // TODO: LOG_DEBUG("regirster!");
    bzero(order, 256);
    // 插入命令
    snprintf(order, 256,
              "INSERT INTO user(username, password) VALUES('%s','%s')",
              name.c_str(), pwd.c_str());
    // TODO: LOG_DEBUG( "%s", order);
    if(mysql_query(sql, order) == 0) {  // 插入一条记录，命令执行成功
        flag = true;
    }
  }
  // TODO: LOG_DEBUG( "UserVerify success!!");
  mysql_free_result(res);  // 释放内存
  return flag;
}