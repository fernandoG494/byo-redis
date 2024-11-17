#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

int main() {
  // socket config
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket()");
  };

  // address binding
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if(rv) {
    die("connect");
  };

  // data stream
  char msg[] = "hello";
  write(fd, msg, strlen(msg));

  char read_buffer[64] = {};
  ssize_t n = read(fd, read_buffer, sizeof(read_buffer) - 1);
  if(n < 0) {
    die("read");
  };

  printf("server says: %s\n", read_buffer);
  close(fd);

  return 0;
};