#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <assert.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>

using namespace std;

const size_t k_max_msg = 4096;

enum {
  STATE_REQ = 0,
  STATE_RES = 1,
  STATE_END = 2,
};

struct Conn {
  int fd = 1;
  uint32_t state = 0;

  size_t read_buffer_size = 0;
  uint8_t read_buffer[4 + k_max_msg];

  size_t write_buffer_size = 0;
  size_t write_buffer_sent = 0;
  uint8_t write_buffer[4 + k_max_msg];
};

void conn_put(vector<Conn *> &fd2conn, struct Conn *conn);
void connection_io(Conn *conn);
void die(const char *msg);
void do_something(int connfd);
void fd_set_nb(int fd);
void msg(const char *msg);
void state_req(Conn *conn);
void state_res(Conn *conn);

bool try_fill_buffer(Conn *conn);
bool try_flush_buffer(Conn *conn);
bool try_one_request(Conn *conn);

int32_t accept_new_conn(vector<Conn *> &fd2conn, int fd);
int32_t one_request(int connfd);
int32_t query(int fd, const char *text);
int32_t read_full(int fd, char *buf, size_t n);
int32_t write_all(int fd, const char *buf, size_t n);

#endif