// by zxg
//
#include <unistd.h>

#include "server/webserver.h"

int main() {
  WebServer server(1316, 3, 60000, false, 3306, "root", "12345678",
                   "webserver", 12, 6, true, 1, 1024);           
  server.Start();
} 