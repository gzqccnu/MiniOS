#include "user.h"

int sys_getc(void) { return (int)sys_call3(SYS_GETC, 0, 0, 0); }
