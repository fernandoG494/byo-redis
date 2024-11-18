#ifndef UTILS_H
#define UTILS_H

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <cassert>

void msg(const char *msg);
void die(const char *msg);
int32_t read_full(int fd, char *buf, size_t n);
int32_t write_all(int fd, const char *buf, size_t n);
int32_t one_request(int connfd);
int32_t query(int fd, const char *text);

#endif