// Log some messages
// by zxg
//
#ifndef WEBSERVER_LOG_LOG_H_
#define WEBSERVER_LOG_LOG_H_

#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir

#include <mutex>
#include <string>
#include <thread>

#include "block_queue.h"
#include "../buffer/buffer.h"

class Log {
 public:
  void Init(int level, const char* path = "./log", const char* suffix =".log",
            int max_capacity = 1024);

  static Log* Instance() {
    static Log inst;
    return &inst;
  }
  
  static void FlushLogThread();

  void Write(int level, const char *format,...);
  void Flush();

  int get_level();
  void set_level(int level);
  inline bool IsOpen() { return is_open_; }
    
 private:
  Log();
  virtual ~Log();

  void AppendLogLevelTitle(int level);
  void AsyncWrite();

  static const int LOG_PATH_LEN = 256;
  static const int LOG_NAME_LEN = 256;
  static const int MAX_LINES = 50000;

  const char* path_;
  const char* suffix_;

  int MAX_LINES_;

  int linecount_;
  int today_;

  bool is_open_;

  Buffer buff_;
  int level_;
  bool is_async_;

  FILE* fp_;
  std::unique_ptr<BlockDeque<std::string>> deque_; 
  std::unique_ptr<std::thread> write_thread_;
  std::mutex mtx_;
};

#define LOG_BASE(level, format, ...) \
  do {\
    Log* log = Log::Instance();\
    if (log->IsOpen() && log->get_level() <= level) {\
      log->Write(level, format, ##__VA_ARGS__); \
      log->Flush();\
    }\
  } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);


#endif  // WEBSERVER_LOG_LOG_H_