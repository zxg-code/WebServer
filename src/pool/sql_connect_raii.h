#ifndef SERVER_POOL_SQL_CONNECT_RAII_H_
#define SERVER_POOL_SQL_CONNECT_RAII_H_

#include "sql_connect_pool.h"

// 数据库连接的构造和释放
// 构造函数获取一个连接，析构函数负责释放，用户不关心如何连接和释放
class SqlConnectionRaii {
 public:
  // Issue: 原版sql似乎是多余的
  SqlConnectionRaii(MYSQL** sql, SqlConnectionPool* conn_pool) {
    assert(conn_pool);
    // sql似乎是多余的
    *sql = conn_pool->GetConnection();
    sql_ = *sql;
    conn_pool_ = conn_pool;
  }

  ~SqlConnectionRaii() {
    if (sql_) conn_pool_->FreeConnection(sql_);
  }

  MYSQL* get_sql() { return sql_; }

 private:
  MYSQL* sql_;
  SqlConnectionPool* conn_pool_;
};

#endif  // SERVER_POOL_SQL_CONNECT_RAII_H_
