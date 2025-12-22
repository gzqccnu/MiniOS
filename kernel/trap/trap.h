#ifndef _TRAP_H_
#define _TRAP_H_

#include <stdint.h>

/* Control trap printing: set to 1 for debug (verbose) mode, 0 for silent mode.
 * Can be overridden by -DTRAP_DEBUG=0 in CFLAGS.
 */
#ifndef TRAP_DEBUG
#define TRAP_DEBUG 0
#endif

// unsigned long read_csr(const char *name);
void trap_init(void);
void trap_handler_c(uint64_t *tf);

#endif /* _TRAP_H_ */
