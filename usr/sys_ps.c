#include "user.h"

int sys_ps(void) { return (int)sys_call3(SYS_PS, 0, 0, 0); }
