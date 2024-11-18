#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void die(const char *msg);
void fd_set_nb(int fd);
void msg(const char *msg);
int32_t one_request(int connfd);
int32_t query(int fd, const char *text);
int32_t read_full(int fd, char *buf, size_t n);
int32_t write_all(int fd, const char *buf, size_t n);

#endif