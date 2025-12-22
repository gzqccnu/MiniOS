#include "user.h"

int sys_ls(struct dirent *ents, int max_ents) {
  return (int)sys_call3(SYS_LS, (uint64_t)ents, (uint64_t)max_ents, 0);
}
