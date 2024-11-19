#include "utils.h"

using namespace std;

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

  // a map of all client connections, keyed by fd
  vector<Conn *> fd2conn;
  fd_set_nb(fd);  // listen fd to nonblocking mode

  vector<struct pollfd> poll_args;
  
  while (true) {
    // prepare arguments
    poll_args.clear();

    struct pollfd pfd = {fd, POLLIN, 0};
    poll_args.push_back(pfd);

    // connection fds
    for (Conn *conn: fd2conn) {
      if (!conn) {
        continue;
      };

      struct pollfd pfd = {};
      pfd.fd = conn->fd;
      pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
      pfd.events = pfd.events | POLLERR;
      poll_args.push_back(pfd);
    };

    // poll for active fds
    int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
    if (rv < 0) {
      die("poll");
    };

    // process active direction
    for (size_t i = 0; i < poll_args.size(); i++) {
      if (poll_args[i].revents) {
        Conn *conn = fd2conn[poll_args[i].fd];
        connection_io(conn);

        if (conn->state == STATE_END) {
          // client closed normally, or something bad happened.
          // destroy this connection
          fd2conn[conn->fd] = NULL;
          (void)close(conn->fd);
          free(conn);
        };
      };
    };
    
    // try to accept a new connection if the listening fd is active
     if (poll_args[0].revents) {
      (void)accept_new_conn(fd2conn, fd);
     };
  };

  return 0;
};
