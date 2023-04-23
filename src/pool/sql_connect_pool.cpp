#include "sql_connect_pool.h"

using namespace std;

SqlConnectionPool::SqlConnectionPool() {
  num_users_ = 0;
  num_free_ = 0;
}

SqlConnectionPool::~SqlConnectionPool() {
  CloseSqlConnPool();
}

SqlConnectionPool* SqlConnectionPool::Instance() {
  static SqlConnectionPool conn_pool;
  return &conn_pool;
}

// 初始化连接池和信号量
void SqlConnectionPool::Init(const char* host, int port, 
                             const char* user, const char* pwd, 
                             const char* db_name, int conn_size = 10) {
  assert(conn_size > 0);
  for (int i = 0; i < conn_size; ++i) {  // 循环创建多条数据库连接
    MYSQL* sql = nullptr;
    sql = mysql_init(sql);  // 初始化一个MYSQL对象，失败则返回NULL
    if (!sql) {
      LOG_ERROR("MySql init error!");
      assert(sql);
    }
    // 建立连接
    // ret: MYSQL* handler if success else NULL
    sql = mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);
    if (!sql) {
      LOG_ERROR("MySql Connect error!");
    }
    sql_conn_que_.emplace(sql);  // 加入连接队列
  } // for
  max_connections_ = conn_size;
  sem_id_ = SemaphoreWrapper(max_connections_);  // 构造信号量，初值为最大连接数
}

// 从数据库连接队列中取出第一个连接供使用，注意线程同步
MYSQL* SqlConnectionPool::GetConnection() {
  MYSQL* conn = nullptr;
  if (sql_conn_que_.empty()) {
    LOG_WARN("SqlConnPool busy!");
    return nullptr;
  }
  sem_id_.Wait();  // 信号量-1
  {
    lock_guard<mutex> locker(mtx_);  // 独占访问
    conn = sql_conn_que_.front();  // 取出队头连接
    sql_conn_que_.pop();
  }
  return conn;
}

// 释放一个当前使用的连接，就是把它再塞到连接队列里去，所以信号量加了，可用的连接加了
void SqlConnectionPool::FreeConnection(MYSQL* sql) {
  assert(sql);  // 要释放的连接必须存在
  lock_guard<mutex> locker(mtx_);
  sql_conn_que_.emplace(sql);
  sem_id_.Post();  // 信号量+1
}

// 获取空闲连接数量
int SqlConnectionPool::GetNumFreeConn() {
  lock_guard<mutex> locker(mtx_);
  return sql_conn_que_.size();
}

// 关闭数据库连接池
void SqlConnectionPool::CloseSqlConnPool() {
  lock_guard<mutex> locker(mtx_);
  while (!sql_conn_que_.empty()) {
    auto conn = sql_conn_que_.front();
    sql_conn_que_.pop();
    mysql_close(conn);  // Closes a previously opened connection.
  }
  mysql_library_end();  // 终止mysql库
}