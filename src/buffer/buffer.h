#ifndef WEBSERVER_BUFFER_BUFFER_H_
#define WEBSERVER_BUFFER_BUFFER_H_

#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>

#include <cstring>
#include <iostream>
#include <vector>
#include <atomic>

class Buffer {
 public:
  Buffer(int buffer_size = 1024);
  Buffer() = delete;
  ~Buffer() = default;

  size_t WriteableBytes() const;        // 可读字节数
  size_t ReadableBytes() const;         // 可写字节数
  // 预留的字节数
  size_t PrependableBytes() const;      // 可预备字节数

  // 数据的起始位置
  const char* Peek() const;
  // 保证有长度为len的空间可写，如果长度不够，需要分配空间
  void EnsureWriteable(size_t len);
  // 移动写指针
  void HasWritten(size_t len);
  // 读取buffer内容后，移动读指针，回收空间？
  void Retrieve(size_t len);
  // 从起始位置移动读指针到某一位置
  void RetrieveUntil(const char* end);
  // 回收buffer
  void RetrieveAll() ;
  // 数据转为字符串，并回收buffer
  std::string RetrieveAllToStr();     
  // 当前开始写的位置
  const char* BeginWrite() const;
  char* BeginWrite();

  // 向缓冲池中写入数据，移动写指针
  void Append(const std::string& str);
  void Append(const char* str, size_t len);
  void Append(const void* data, size_t len);
  void Append(const Buffer& buff);

  ssize_t ReadFd(int fd, int* save_errno);   // 读接口
  ssize_t WriteFd(int fd, int* save_errno);  // 写接口

 private:
  // 缓冲区初始位置的指针
  char* BeginPtr();  
  const char* BeginPtr() const;
  // resize buffer or use prependable area
  void MakeSpace(size_t len);

  std::vector<char> buffer_;            // 存储实体
  std::atomic<std::size_t> read_pos_;   // 读指针位置，原子类型的size_t
  std::atomic<std::size_t> write_pos_;  // 写指针位置 (its offset from a adderss)
};

#endif  // WEBSERVER_BUFFER_BUFFER_H_