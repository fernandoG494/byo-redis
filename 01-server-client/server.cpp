#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

using namespace std;

static void do_something(int connfd) {
  char read_buffer[64] = {};
  ssize_t n = read(connfd, read_buffer, sizeof(read_buffer) - 1);

  if (n < 0) {
    msg("read() error");
    return;
  };

  printf("client says: %s\n", read_buffer);

  char write_buffer[] = "world";
  write(connfd, write_buffer, strlen(write_buffer));
};

int main() {
  // configuracion de socket
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket()");
  };

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // address binding
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(0);

  int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  };

  rv = listen(fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  };

  // loop for connection
  while (true) {
    // accept connections
    struct sockaddr_in client_address = {};
    socklen_t socklen = sizeof(client_address);
    int connfd = accept(fd, (struct sockaddr *)&client_address, &socklen);
    if (connfd < 0) {
      continue; // error
    };

    while (true) {
      int32_t error = one_request(connfd);
      if (error) {
        break;
      };
    };

    close(connfd);
  };

  return 0;
};
