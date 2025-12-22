#include "user.h"

long sys_sleep(unsigned long ticks) { return (long)sys_call3(SYS_SLEEP, ticks, 0, 0); }
