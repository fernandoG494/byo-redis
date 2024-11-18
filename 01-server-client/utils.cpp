#include "utils.h"

const size_t k_max_msg = 4096;

void msg(const char *msg) {
  printf("%sn", msg);
};

void die(const char *msg) {
  int error = errno;
  printf("[%d]: %s\n", error, msg);
  abort();
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