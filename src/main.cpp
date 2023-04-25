// by zxg
//
#include <unistd.h>

#include "server/webserver.h"

int main() {
  int port = 1316;
  int trig_mode = 3;
  int timeout = 60000;
  bool linger = false;
  char* database = "webserver";
  int num_sql_conn = 9;
  int num_threads = 6;
  bool log = true;
  int log_level = 1;
  WebServer server(port, trig_mode, timeout, linger,
                   3306, "root", "12345678", database, num_sql_conn,  // mysql settings
                   num_threads, log, log_level, 1024);           
  server.Start();
} 