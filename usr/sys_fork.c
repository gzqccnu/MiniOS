#include "user.h"

int sys_fork(void) { return (int)sys_call3(SYS_FORK, 0, 0, 0); }
