#include "utils_v2.h"

void msg(const char *msg) {
  printf("%sn", msg);
};

void die(const char *msg) {
  int error = errno;
  printf("[%d]: %s\n", error, msg);
  abort();
};