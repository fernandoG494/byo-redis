#include "utils.h"

void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn) {
  if (fd2conn.size() <= (size_t)conn->fd) {
    fd2conn.resize(conn->fd + 1);
  }
  fd2conn[conn->fd] = conn;
};

void connection_io(Conn *conn) {
  if (conn->state == STATE_REQ) {
    state_req(conn);
  } else if (conn->state == STATE_RES) {
    state_res(conn);
  } else {
    assert(0);  // not expected
  };
};

void die(const char *msg) {
  int error = errno;
  printf("[%d]: %s\n", error, msg);
  abort();
};

void do_something(int connfd) {
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

void fd_set_nb(int fd) {
  errno = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    die("fcntl error");
    return;
  };

  flags |= O_NONBLOCK;

  errno = 0;
  (void)fcntl(fd, F_SETFL, flags);
  if (errno) {
    die("fcntl error");
  };
};

void msg(const char *msg) {
  printf("%sn", msg);
};

void state_req(Conn *conn) {
  while (try_fill_buffer(conn)) {}
};

void state_res(Conn *conn) {
  while (try_flush_buffer(conn)) {}
};

bool try_fill_buffer(Conn *conn) {
  // try to fill the buffer
  assert(conn->read_buffer_size < sizeof(conn->read_buffer_size));
  ssize_t rv = 0;
  do {
    size_t cap = sizeof(conn->read_buffer) - conn->read_buffer_size;
    rv = read(conn->fd, &conn->read_buffer[conn->read_buffer_size], cap);
  } while (rv < 0 && errno == EINTR);
  if (rv < 0 && errno == EAGAIN) {
    // got EAGAIN, stop.
    return false;
  }
  if (rv < 0) {
    msg("read() error");
    conn->state = STATE_END;
    return false;
  }
  if (rv == 0) {
    if (conn->read_buffer_size > 0) {
      msg("unexpected EOF");
    } else {
      msg("EOF");
    }
    conn->state = STATE_END;
    return false;
  }

  conn->read_buffer_size += (size_t)rv;
  assert(conn->read_buffer_size <= sizeof(conn->read_buffer));

  // Try to process requests one by one.
  // Why is there a loop? Please read the explanation of "pipelining".
  while (try_one_request(conn)) {}
  return (conn->state == STATE_REQ);
};

bool try_flush_buffer(Conn *conn) {
  ssize_t rv = 0;
  do {
    size_t remain = conn->write_buffer_size - conn->write_buffer_sent;
    rv = write(conn->fd, &conn->write_buffer[conn->write_buffer_sent], remain);
  } while (rv < 0 && errno == EINTR);
  if (rv < 0 && errno == EAGAIN) {
    // got EAGAIN, stop.
    return false;
  }
  if (rv < 0) {
    msg("write() error");
    conn->state = STATE_END;
    return false;
  }
  conn->write_buffer_sent += (size_t)rv;
  assert(conn->write_buffer_sent <= conn->write_buffer_size);
  if (conn->write_buffer_sent == conn->write_buffer_size) {
    // response was fully sent, change state back
    conn->state = STATE_REQ;
    conn->write_buffer_sent = 0;
    conn->write_buffer_size = 0;
    return false;
  };
  // still got some data in wbuf, could try to write again
  return true;
}

bool try_one_request(Conn *conn) {
  // try to parse a request from the buffer
  if (conn->read_buffer_size < 4) {
    // not enough data in the buffer. Will retry in the next iteration
    return false;
  }
  uint32_t len = 0;
  memcpy(&len, &conn->read_buffer[0], 4);
  if (len > k_max_msg) {
    msg("too long");
    conn->state = STATE_END;
    return false;
  }
  if (4 + len > conn->read_buffer_size) {
    // not enough data in the buffer. Will retry in the next iteration
    return false;
  }

  // got one request, do something with it
  printf("client says: %.*s\n", len, &conn->read_buffer[4]);

  // generating echoing response
  memcpy(&conn->write_buffer[0], &len, 4);
  memcpy(&conn->write_buffer[4], &conn->read_buffer[4], len);
  conn->write_buffer_size = 4 + len;

  // remove the request from the buffer.
  // note: frequent memmove is inefficient.
  // note: need better handling for production code.
  size_t remain = conn->read_buffer_size - 4 - len;
  if (remain) {
    memmove(conn->read_buffer, &conn->read_buffer[4 + len], remain);
  }
  conn->read_buffer_size = remain;

  // change state
  conn->state = STATE_RES;
  state_res(conn);

  // continue the outer loop if the request was fully processed
  return (conn->state == STATE_REQ);
};

int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
  // accept
  struct sockaddr_in client_addr = {};
  socklen_t socklen = sizeof(client_addr);
  int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
  if (connfd < 0) {
    msg("accept() error");
    return -1;  // error
  }

  // set the new connection fd to nonblocking mode
  fd_set_nb(connfd);
  // creating the struct Conn
  struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
  if (!conn) {
    close(connfd);
    return -1;
  }
  conn->fd = connfd;
  conn->state = STATE_REQ;
  conn->read_buffer_size = 0;
  conn->write_buffer_size = 0;
  conn->write_buffer_sent = 0;
  conn_put(fd2conn, conn);
  return 0;
}

int32_t one_request(int connfd) {
  // 4 bytes header
  char read_buffer[4 + k_max_msg + 1];
  errno = 0;

  int32_t err = read_full(connfd, read_buffer, 4);
  if (err) {
    if (errno == 0) {
      msg("EOF");
    } else {
      msg("read() error");
    };

    return err;
  };

  u_int32_t len = 0;
  memcpy(&len, read_buffer, 4);
  if (len > k_max_msg) {
    msg("too long");
    return -1;
  };

  // request body
  err = read_full(connfd, &read_buffer[4], len);
  if (err) {
    msg("read() error");
    return err;
  };

  // do something
  read_buffer[4 + len] = '\0';
  printf("client says: %s\n", &read_buffer[4]);

  // reply using same protocol
  const char reply[] = "world";
  char write_buffer[4 + sizeof(reply)];
  len = (u_int32_t)strlen(reply);

  memcpy(write_buffer, &len, 4);
  memcpy(&write_buffer[4], reply, len);

  return write_all(connfd, write_buffer, 4 + len);
};

int32_t query(int fd, const char *text) {
  u_int32_t len = (u_int32_t)strlen(text);
  if(len > k_max_msg) {
    return -1;
  };

  char write_buffer[4 + k_max_msg];
  memcpy(write_buffer, &len, 4);
  memcpy(&write_buffer[4], text, len);
  if (int32_t err = write_all(fd, write_buffer, 4 + len)) {
    return err;
  };

  // 4 bytes header
  char read_buffer[4 + k_max_msg + 1];
  errno = 0;
  int32_t err = read_full(fd, read_buffer, 4);
  if (err) {
    if (errno == 0) {
      msg("EOF");
    } else {
      msg("read() error");
    };
    return err;
  };

  memcpy(&len, read_buffer, 4);
  if (len > k_max_msg) {
    msg("too long");
    return -1;
  };

  // reply body
  err = read_full(fd, &read_buffer[4], len);
  if (err) {
    msg("read() error");
    return err;
  };

  // do something
  read_buffer[4 + len] = '\0';
  printf("server says: %s\n", &read_buffer[4]);
  return 0;
};

int32_t read_full(int fd, char *buf, size_t n) {
  while(n > 0) {
    ssize_t rv = read(fd, buf, n);
    if(rv <= 0) {
      return -1; // error or unexpected EOF
    };

    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  };

  return 0;
};

int32_t write_all(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      return -1; // error o EOF
    };

    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  };

  return 0;
};
