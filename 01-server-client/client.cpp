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

  // multiple requests
  int32_t err = query(fd, "hello 1");
  if (err) {
    goto L_DONE;
  };
  err = query(fd, "hello 2");
  if (err) {
    goto L_DONE;
  };
  err = query(fd, "hello 3");
  if (err) {
    goto L_DONE;
  };
  L_DONE: 
    close(fd);

  return 0;
};