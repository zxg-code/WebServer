#include "http_response.h"

using namespace std;

const unordered_map<string, string> HttpResponse::suffix_type_ = { 
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::code_status_ = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::code_path_ = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() : code_(-1), path_(""), src_dir_(""),
                               is_keep_alive_(false), mm_file_(nullptr),
                               mm_file_stat_({0}) {};

HttpResponse::~HttpResponse() { UnmapFile(); }
// 含参初始化函数
void HttpResponse::Init(const string& src_dir, string& path, bool is_keep_alive, int code){
  assert(src_dir != "");
  if(mm_file_) { UnmapFile(); }
  code_ = code;
  is_keep_alive_ = is_keep_alive;
  path_ = path;
  src_dir_ = src_dir;
  mm_file_ = nullptr; 
  mm_file_stat_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer* buff) {
  // 判断请求的资源文件
  // stat获取文件的状态信息，并写入stat结构体变量中
  // data方法将一个string变成c-string，以空字符结尾，c++11之后，c_str和data函数相同作用
  if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)) {
    code_ = 404;  // 路径为目录则404
  } else if (!(mm_file_stat_.st_mode & S_IROTH)) {  // IR: 读权限, OTH: 其他用户
    code_ = 403;  // 不具有读权限
  } else if(code_ == -1) { 
    code_ = 200; 
  }
  ErrorHtml();
  AddStateLine(buff);
  AddHeader(buff);
  AddContent(buff);
}

void HttpResponse::ErrorHtml() {
  if (code_path_.count(code_)) {
    path_ = code_path_.at(code_);
    stat((src_dir_ + path_).data(), &mm_file_stat_);
  }
}

void HttpResponse::AddStateLine(Buffer* buff) {
  // 状态码存在，查找返回；不存在，一律按400返回
  if (code_status_.count(code_) == 0) code_ = 400;  // 400 : bad_request
  auto status = code_status_.at(code_);
  // 写入缓冲池
  buff->Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer* buff) {
  buff->Append("Connection: ");
  if (is_keep_alive_) {
    buff->Append("keep-alive\r\n");
    buff->Append("keep-alive: max=6, timeout=120\r\n");
  } else buff->Append("close\r\n");
  buff->Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer* buff) {
  // 只读方式打开文件
  int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
  if (src_fd < 0) { 
    ErrorContent(buff, "File NotFound!");
    return; 
  }
  // 将文件映射到内存提高文件的访问速度 MAP_PRIVATE 建立一个写入时拷贝的私有映射
  // TODO: LOG_DEBUG("file path %s", (srcDir_ + path_).data());
  int* mmRet = static_cast<int*>(mmap(0, mm_file_stat_.st_size, PROT_READ,
                                      MAP_PRIVATE, src_fd, 0));
  if (*mmRet == -1) {  // 映射文件失败
    ErrorContent(buff, "File NotFound!");
    return; 
  }
  mm_file_ = (char*)mmRet;  // 映射的地址保存起来
  close(src_fd);
  // 写入缓存
  buff->Append("Content-length: " + to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile() {
  if (mm_file_) {
    munmap(mm_file_, mm_file_stat_.st_size);
    // mm_file_ = nullptr;
  }
  delete mm_file_;
}

string HttpResponse::GetFileType() {
  string::size_type idx = path_.find_last_of('.');
  if (idx == string::npos) return "text/plain";  // 没有后缀名
  auto suffix = path_.substr(idx);  // ".xxx"
  if (suffix_type_.count(suffix) == 1) return suffix_type_.at(suffix);
  return "text/plain";  // 后缀名不是已知的类型
}

void HttpResponse::ErrorContent(Buffer* buff, string message) {
  // 如果有未定义的状态码，按 (400:bad_request) 处理
  if (code_status_.count(code_) == 0) code_ = 400; 
  string status = code_status_.at(code_);
  string body = "";

  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";

  body += to_string(code_) + " : " + status  + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";

  buff->Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
  buff->Append(body);
}