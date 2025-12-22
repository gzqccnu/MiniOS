#include "user.h"

int sys_getpid(void) { return (int)sys_call3(SYS_GETPID, 0, 0, 0); }
