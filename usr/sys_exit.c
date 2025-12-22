#include "user.h"

void sys_exit(int code) { (void)sys_call3(SYS_EXIT, (uint64_t)code, 0, 0); }
