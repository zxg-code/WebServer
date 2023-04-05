#ifndef SERVER_POOL_SQL_CONNECT_POOL_H_
#define SERVER_POOL_SQL_CONNECT_POOL_H_

#include <semaphore.h>

#include <string>
#include <queue>
#include <mutex>
#include <thread>

#include <mysql/mysql.h>  

#include <src/lock/locker.h>

// #include  // log.h

// 数据库连接池
class SqlConnectionPool {
 public:
  static SqlConnectionPool* Instance();  // 单例模式

  MYSQL* GetConnection();
  void FreeConnection(MYSQL* conn);
  int GetNumFreeConn();
  
  void Init(const char* host, int port, const char* user,
            const char* pwd, const char* db_name, int conn_size);
  void CloseSqlConnPool();

 private:
  SqlConnectionPool();
  ~SqlConnectionPool();

  int max_connections_;              // 最大连接数
  int num_users_;                    // 已使用连接数
  int num_free_;                     // 空闲连接数
  std::queue<MYSQL*> sql_conn_que_;  // 连接池
  std::mutex mtx_;
  // sem_t sem_id_;                     // 信号量
  SemaphoreWrapper sem_id_;
}

#endif  // SERVER_POOL_SQL_CONNECT_POOL_H_
