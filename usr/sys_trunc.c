#include "user.h"

int sys_trunc(const char *name) { return (int)sys_call3(SYS_TRUNC, (uint64_t)name, 0, 0); }
