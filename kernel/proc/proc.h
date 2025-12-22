// proc.h

#ifndef _PROC_H_
#define _PROC_H_

#include "../include/types.h"
#include <stddef.h>

// process state
typedef enum ProcessState { READY = 0, RUNNING, BLOCKED, TERMINATED } ProcState;

// forward declare for pointer type
typedef struct ProcessControlBlock PCB;

// define PCB
struct ProcessControlBlock {
  int pid;              // process id
  ProcState pstat;      // process state
  char name[20];        // process name
  int prior;            // priority (lower = higher priority)
  uint64_t entrypoint;  // entry point (instruction address)
  uint64_t stacktop;    // stack top virtual address
  int ppid;             // parent pid (0 for kernel/init)
  void *brk_base;       // program break base (heap)
  uint64_t brk_size;    // allocated heap size in bytes
  uint64_t cpu_time;    // cpu consumed time
  uint64_t remain_time; // remaining time slice
  uint64_t arriv_time;  // arrival time
  RegState regstat;     // saved register state for context switch
  PCB *next;            // link list pointer, for queue managing
};

// define process queue
typedef struct ProcessQueue {
  PCB *head; // queue head
  PCB *tail; // queue tail
  int count; // queue count
} procqueue;

// APIs
procqueue *init_procqueue(void);
void enqueue(procqueue *queue, PCB *pcb);
PCB *dequeue(procqueue *queue);

// process management
PCB *proc_create(const char *name, uint64_t entrypoint, int prior);
void proc_exit(void);
void scheduler_init(void);
void schedule(void);
PCB *get_current_proc(void);
/* fork current process: return child's pid, or -1 on error */
PCB *proc_fork(uint64_t mepc);
/* wait for a child in zombie list and reap it; return pid or -1 if none */
int proc_wait_and_reap(void);

// debug: dump all processes and their states
void proc_dump(void);

extern procqueue *ready_queue;
extern PCB *current_proc;

#endif /* _PROC_H_ */
