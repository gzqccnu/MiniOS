#include "user.h"

int sys_wait(void) { return (int)sys_call3(SYS_WAIT, 0, 0, 0); }
