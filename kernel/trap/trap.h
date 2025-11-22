#ifndef _TRAP_H_
#define _TRAP_H_

static inline unsigned long read_csr(const char *name);
void trap_init(void);
void trap_handler_c(void);

#endif /* _TRAP_H_ */