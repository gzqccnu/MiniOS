#include "user.h"

int sys_close(int fd) { return (int)sys_call3(SYS_CLOSE, (uint64_t)fd, 0, 0); }
