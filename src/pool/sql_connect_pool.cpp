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
      // TODO: log error
      assert(sql);
    }
    // 建立连接
    // ret: MYSQL* handler if success else NULL
    sql = mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);
    if (!sql) {
      // TODO: log error
    }
    sql_conn_que_.emplace(sql);  // 加入连接队列
  } // for
  max_connections_ = conn_size;
  sem_id_ = SemaphoreWrapper(max_connections_);  // 构造信号量，初值为最大连接数
}

// 从数据库连接队列中取出第一个，注意线程同步
MYSQL* SqlConnectionPool::GetConnection() {
  MYSQL* conn = nullptr;
  if (sql_conn_que_.empty()) {
    // TODO: log warnning of "sqlconnpool busy"
    return nullptr;
  }
  sem_id_.Wait();  // 信号量-1
  {
    lock_guard<mutex> locker(mtx_);  // 独占访问
    conn = sql_conn_que_.front();
    sql_conn_que_.pop();
  }
  return conn;
}

// 