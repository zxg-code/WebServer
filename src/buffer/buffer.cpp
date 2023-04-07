#include "buffer.h"

Buffer::Buffer(int buffer_size) 
    : buffer_(buffer_size), read_pos_(0), write_pos_(0) {}

size_t Buffer::ReadableBytes() const {
  return write_pos_ - read_pos_;  
}

size_t Buffer::WriteableBytes() const {
  return buffer_.size() - write_pos_;
}

size_t Buffer::PrependableBytes() const {
  return read_pos_;  // 并不是从0开始操作，而是从read_pos开始，留出[0, read_pos]这段空间
}

// begin address
char* Buffer::BeginPtr() {
  return &*buffer_.begin();  // convert iterator to pointer
}
// begin address
const char* Buffer::BeginPtr() const {
  return &*buffer_.begin();
}

// read address = begin address + read offset
const char* Buffer::Peek() const {
  return BeginPtr() + read_pos_;   
}

// write address = begin address + write offset
const char* Buffer::BeginWrite() const {
  return BeginPtr() + write_pos_; 
}
// write address = begin address + write offset
char* Buffer::BeginWrite() {
  return BeginPtr() + write_pos_;
}

void Buffer::HasWritten(size_t len) {  // TODO: only use twice
  write_pos_ += len;  // write offset
}

void Buffer::Retrieve(size_t len) {  // TODO: not use
  assert(len <= ReadableBytes());
  read_pos_ += len;  // read offset 
}

void Buffer::RetrieveUntil(const char* end) {
  auto start = Peek();
  assert(start <= end);
  Retrieve(end - start);  // from cur address to a specific address
}

void Buffer::RetrieveAll() {
  bzero(&buffer_[0], buffer_.size());
  read_pos_ = 0;  // 读写指针归零
  write_pos_ = 0;  // buffer was empty
}

std::string Buffer::RetrieveAllToStr() {
  std::string str(Peek(), ReadableBytes());  // to_string
  RetrieveAll();
  return str;
}

void Buffer::EnsureWriteable(size_t len) {
  auto wb = WriteableBytes();
  if (wb < len) {
    MakeSpace(len);  // 空间不够需要再分配
  }
  assert(wb >= len);
}

void Buffer::MakeSpace(size_t len) {
  if (WriteableBytes() + PrependableBytes() < len) {  // 加上预留区都不够
    buffer_.resize(write_pos_ + len + 1);
  } else {
    size_t readable = ReadableBytes();
    auto start = BeginPtr();
    // offset a block
    std::copy(start + read_pos_, start + write_pos_, start);  
    read_pos_ = 0;  // prependable area was used
    write_pos_ = readable;  // wirte_pos = read_pos + readable
    assert(readable == ReadableBytes());  // validate
  }
}

void Buffer::Append(const char* str, size_t len) {
  assert(str);
  EnsureWriteable(len);
  std::copy(str, str+len, BeginWrite());  // 写入
  HasWritten(len);                        // 移动写指针
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
} 

ssize_t Buffer::ReadFd(int fd, int* save_errno) {
  char buff[65535];
  struct iovec iov[2];  // two buffers
  const size_t writeable = WriteableBytes();
  iov[0].iov_base = BeginWrite();  // first buff
  iov[0].iov_len = writeable;
  iov[1].iov_base = buff;          // second buff
  iov[1].iov_len = sizeof(buff);
  // read two buffers
  const ssize_t n = readv(fd, iov, 2);  // ret: num of bytes read or -1
  if (n < 0) {
    *save_errno = errno;
    return n;  // -1
  }

  if (static_cast<size_t>(n) <= writeable) {
    write_pos_ += n;
  } else {
    write_pos_ = buffer_.size();  // first buff is full
    Append(buff, n - writeable);  // resize first buff
  }
  return n;
}

ssize_t Buffer::WriteFd(int fd, int* save_errno) {
  auto readable = ReadableBytes();
  ssize_t n = write(fd, Peek(), readable);
  if (n < 0) {
    *save_errno = errno;
    return n;  // -1
  }
  read_pos_ += n;
  return n;
}