// Implementation of log
// by zxg
//
#include "log.h"

using namespace std;

Log::Log()
    : linecount_(0), 
      is_async_(false), 
      write_thread_(nullptr), 
      deque_(nullptr),
      today_(0),
      fp_(nullptr) {}

Log::~Log() {
  if (write_thread_ && write_thread_->joinable()) {
    while (!deque_->Empty()) {
      deque_->Flush();
    }
    deque_->Close();
    write_thread_->join();
  }
  if (fp_) {
    lock_guard<mutex> locker(mtx_);
    Flush();
    fclose(fp_);
  }
}

void Log::Init(int level, const char* path, const char* suffix,
               int max_capacity) {
  is_open_ = true;
  level_ = level;
  if (max_capacity > 0) {
    is_async_ = true;
    if (!deque_) {
      deque_ = make_unique<BlockDeque<string>>(); 
      write_thread_ = make_unique<thread>(FlushLogThread);
    }
  } else is_async_ = false;

  linecount_ = 0;
  time_t timer = time(nullptr);
  struct tm *sysTime = localtime(&timer);
  struct tm t = *sysTime;
  path_ = path;
  suffix_ = suffix;
  char fileName[LOG_NAME_LEN] = {0};
  snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
          path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  today_ = t.tm_mday;

  {
    lock_guard<mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) { 
      Flush();
      fclose(fp_); 
    }
    fp_ = fopen(fileName, "a");
    if(fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(fileName, "a");
    } 
    assert(fp_ != nullptr);
  }
}

int Log::get_level() {
  lock_guard<mutex> locker(mtx_);
  return level_;
}

void Log::set_level(int level) {
  lock_guard<mutex> locker(mtx_);
  level_ = level;
}

void Log::Write(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm *sysTime = localtime(&tSec);
  struct tm t = *sysTime;
  va_list vaList;

  /* 日志日期 日志行数 */
  if (today_ != t.tm_mday || (linecount_ && (linecount_ % MAX_LINES == 0))) {
    unique_lock<mutex> locker(mtx_);
    locker.unlock();
    
    char newFile[LOG_NAME_LEN];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if (today_ != t.tm_mday) {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
      today_ = t.tm_mday;
      linecount_ = 0;
    } else {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail,
               (linecount_  / MAX_LINES), suffix_);
    }
    
    locker.lock();
    Flush();
    fclose(fp_);
    fp_ = fopen(newFile, "a");
    assert(fp_ != nullptr);
  }

  {
    unique_lock<mutex> locker(mtx_);
    ++linecount_;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                     t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                     t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                
    buff_.HasWritten(n);
    AppendLogLevelTitle(level);

    va_start(vaList, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WriteableBytes(), format, vaList);
    va_end(vaList);

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    if (is_async_ && deque_ && !deque_->Full()) {
      deque_->PushBack(buff_.RetrieveAllToStr());
    } else fputs(buff_.Peek(), fp_);
    buff_.RetrieveAll();
  }
}

void Log::AppendLogLevelTitle(int level) {
  switch (level) {
    case 0:
      buff_.Append("[debug]: ", 9);
      break;
    case 1:
      buff_.Append("[info] : ", 9);
      break;
    case 2:
      buff_.Append("[warn] : ", 9);
      break;
    case 3:
      buff_.Append("[error]: ", 9);
      break;
    default:
      buff_.Append("[info] : ", 9);
      break;
  }
}

void Log::Flush() {
  if (is_async_) deque_->Flush(); 
  fflush(fp_);
}

void Log::AsyncWrite() {
  string str = "";
  while (deque_->Pop(str)) {
    lock_guard<mutex> locker(mtx_);
    fputs(str.c_str(), fp_);
  }
}

void Log::FlushLogThread() {
  Log::Instance()->AsyncWrite();
}