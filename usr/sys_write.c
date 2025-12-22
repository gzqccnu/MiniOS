#include "user.h"

long sys_write(int fd, const void *buf, uint64_t len) {
  return (long)sys_call3(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, len);
}
