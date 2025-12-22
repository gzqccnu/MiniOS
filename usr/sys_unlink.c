#include "user.h"

int sys_unlink(const char *name) { return (int)sys_call3(SYS_UNLINK, (uint64_t)name, 0, 0); }
