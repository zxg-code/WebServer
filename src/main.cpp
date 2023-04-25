// by zxg
//
#include <unistd.h>

#include <cstdio>

#include "server/webserver.h"

int main(int argc, char* argv[]) {
  int port = 9000;
  int trig_mode = 3;  // ET
  int timeout = 60000;
  bool linger = false;
  char* database = "webserver";
  int num_sql_conn = 9;
  int num_threads = 6;
  bool log = true;
  int log_level = 1;

  int opt = 0;
  while ((opt = getopt(argc, argv, "p:m:s:t:lo")) != -1) {
    switch (opt) {
      case 'p':  // port
        port = atoi(optarg);
        break;
      case 'm':
        trig_mode = atoi(optarg);
        break;
      case 's':
        num_sql_conn = atoi(optarg);
        break;
      case 't':
        num_threads = atoi(optarg);
        break;
      case 'l':
        linger = true;
        break;
      case 'o':  // 关闭日志
        log = false;
        break;
      case '?':
        printf("Invalid option -- '%c'.\n", (char)optopt);
        printf("Usage: ./bin/server [-p port] [-m trig_mode]"
               " [-s num_sql_conn] [-t num_threads] [-l (turn on opt_linger)]"
               " [-o (trun off log)]\n");
        exit(EXIT_FAILURE);
        break;
      default:
        break;
    }
  }
  WebServer server(port, trig_mode, timeout, linger,
                   3306, "root", "12345678", database, num_sql_conn,  // mysql settings
                   num_threads, log, log_level, 1024);           
  server.Start();
} 