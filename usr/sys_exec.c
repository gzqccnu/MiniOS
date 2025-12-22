#include "user.h"

// simplified exec: on success, does not return (process image replaced)
int sys_exec(const char *name) { return (int)sys_call3(SYS_EXEC, (uint64_t)name, 0, 0); }
