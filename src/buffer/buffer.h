#ifndef WEBSERVER_BUFFER_BUFFER_H_
#define WEBSERVER_BUFFER_BUFFER_H_
// C header
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>
// C++ header
#include <cstring>
#include <iostream>
#include <vector>
#include <atomic>

class Buffer {
 public:
  Buffer(int buffer_size = 1024);
  Buffer() = delete;
  ~Buffer() = default;

  // 可读字节数
  inline size_t WriteableBytes() const {
    return buffer_.size() - write_pos_;
  }

  // 可写字节数
  inline size_t ReadableBytes() const {
    return write_pos_ - read_pos_;  
  }

  // 预留的字节数，from 0 to read_pos_
  inline size_t PrependableBytes() const {
    return read_pos_;  // 并不是从0开始操作，而是从read_pos开始，留出[0, read_pos]这段空间
  }

  // 数据的起始地址 read address, equals begin address + read offset
  inline const char* Peek() const {
    return BeginPtr() + read_pos_;
  }

  // 移动写指针
  inline void HasWritten(size_t len) {
    write_pos_ += len;  // write offset
  }

  // 读取buffer内容后，移动读指针，回收空间？
  inline void Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    read_pos_ += len;  // read offset 
  }

  // const beginwrite
  inline const char* BeginWrite() const {
    return static_cast<const char*>(BeginPtr() + write_pos_); 
  }
  // 当前开始写的位置 write address = begin address + write offset
  inline char* BeginWrite() {
    return BeginPtr() + write_pos_;
  }

  // 保证有长度为len的空间可写，如果长度不够，需要分配空间
  void EnsureWriteable(size_t len);
  // 从起始位置移动读指针到某一位置
  void RetrieveUntil(const char* end);
  // 回收buffer
  void RetrieveAll() ;
  // 数据转为字符串，并回收buffer
  std::string RetrieveAllToStr();     

  // 向缓冲池中写入数据，移动写指针
  void Append(const std::string& str);
  void Append(const char* str, size_t len);
  void Append(const void* data, size_t len);
  void Append(const Buffer& buff);

  ssize_t ReadFd(int fd, int* save_errno);   // 读接口
  ssize_t WriteFd(int fd, int* save_errno);  // 写接口

 private:
  // 缓冲区初始位置的指针, begin adderss
  inline char* BeginPtr() {
    return &*buffer_.begin();  // convert iterator to pointer
  }
  // begin adderss
  inline const char* BeginPtr() const {
    return static_cast<const char*>(&*buffer_.begin());
  }
  // resize buffer or use prependable area
  void MakeSpace(size_t len);

  std::vector<char> buffer_;            // 存储实体
  std::atomic<std::size_t> read_pos_;   // 读指针位置，原子类型的size_t
  std::atomic<std::size_t> write_pos_;  // 写指针位置 (its offset from a adderss)
};

#endif  // WEBSERVER_BUFFER_BUFFER_H_