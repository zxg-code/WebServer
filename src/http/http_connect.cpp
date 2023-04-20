#include "http_connect.h"

using namespace std;

// 静态成员变量
const char* HttpConnect::src_dir;
std::atomic<int> HttpConnect::user_count;
bool HttpConnect::is_ET;

HttpConnect::HttpConnect() : fd_(-1), addr_({0}), is_close_(true) {}

HttpConnect::~HttpConnect() {
  Close();
}

void HttpConnect::Init(int fd, const sockaddr_in& addr) {
  assert(fd > 0);
  user_count++;
  addr_ = addr;
  fd_ = fd;
  iov_cnt_ = 2;  // iov缓冲池数
  // 初始化缓冲池读写位置
  write_buff_->RetrieveAll();
  read_buff_->RetrieveAll();
  is_close_ = false;
  // TODO: LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), 
  //                GetPort(), (int)userCount);
}

void HttpConnect::Close() {
  response_.UnmapFile();
  if (is_close_ == false){
    is_close_ = true; 
    user_count--;
    close(fd_);  // 
    // TODO: LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(),
    //                GetPort(), (int)userCount);
  }
}

bool HttpConnect::Process() {
    request_.Init();
    if (read_buff_->ReadableBytes() <= 0) return false;  // 是否存在可读数据
    else if (request_.Parse(read_buff_)) {
      // TODO: LOG_DEBUG("%s", request_.path().c_str());
      response_.Init(src_dir, request_.get_path(), request_.IsKeepAlive(), 200);
    } else {
      response_.Init(src_dir, request_.get_path(), false, 400);
    }

    response_.MakeResponse(write_buff_);  // 组建响应报文放入写缓冲池
    // 响应头
    iov_[0].iov_base = const_cast<char*>(write_buff_->Peek());
    iov_[0].iov_len = write_buff_->ReadableBytes();
    iov_cnt_ = 1;

    // 需要返回服务器资源，则从映射地址上读取
    if (response_.FileLen() > 0  && response_.get_mm_file()) {
      iov_[1].iov_base = response_.get_mm_file();
      iov_[1].iov_len = response_.FileLen();
      iov_cnt_ = 2;
    }
    // TODO: LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iov_cnt_, ToWriteBytes());
    return true;
}

ssize_t HttpConnect::Read(int* save_errno) {
  ssize_t len = -1;
  // 如果是LT模式，那么只读取一次，如果是ET模式，会一直读取，直到读不出数据
  do {
    len = read_buff_->ReadFd(fd_, save_errno);
    if (len <= 0) break;
  } while (is_ET);
  return len;
}

ssize_t HttpConnect::Write(int* save_errno) {
  ssize_t len = -1;
  do {
    // If successful, writev() returns the number of bytes written from the buffer.
    // else return -1
    len = writev(fd_, iov_, iov_cnt_);
    if (len <= 0) {  // 如果缓冲池是空的或操作失败
      *save_errno = errno;
      break;
    }
    if (iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } // 传输结束
    else if (static_cast<size_t>(len) > iov_[0].iov_len) {
      // 如果发送的数据长度大于iov_[0].iov_len，第一块区域的数据发送完毕，并且第二块也有部分被发送了
      iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);  // 调整开始位置
      iov_[1].iov_len -= (len - iov_[0].iov_len);  // 调整第二块剩余长度
      if (iov_[0].iov_len) {  // 第一块发送完毕，回收空间
        write_buff_->RetrieveAll();
        iov_[0].iov_len = 0;
      }
    }
    else {
      // 第一块还未传输完
      iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
      iov_[0].iov_len -= len; 
      write_buff_->Retrieve(len);  // 回收已读空间
    }
  } while(is_ET || ToWriteBytes() > 10240);
  return len;
}