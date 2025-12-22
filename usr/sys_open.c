#include "user.h"

int sys_open(const char *name, int create) {
  return (int)sys_call3(SYS_OPEN, (uint64_t)name, (uint64_t)create, 0);
}
