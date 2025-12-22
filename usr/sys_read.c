#include "user.h"

long sys_read(int fd, void *buf, uint64_t len) {
  return (long)sys_call3(SYS_READ, (uint64_t)fd, (uint64_t)buf, len);
}
