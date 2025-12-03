// proc.c

#include "proc.h"
#include "../include/log.h"
#include "../include/riscv.h"
#include "../mem/kmem.h"
#include "../string/string.h"

// extern assembly context switch
extern void switch_context(RegState *old, RegState *new);
extern void forkret(void);

// globals
PCB *idle_proc = NULL; // global Idle process pointer
procqueue *ready_queue = NULL;
PCB *current_proc = NULL;
PCB *zombie_list = NULL; // zombie process

static int next_pid = 1;
static RegState boot_ctx; // temporary context for boot / first switch

// Idle 进程的入口函数
void idle_entry(void) {
  // 1. Make sure interrupts are enabled (MIE=1).
  // Although they should already be enabled when forkret or schedule returns, explicitly enable them just to be safe.
  // 2. Execute wfi to wait for an interrupt.
  while (1) {
    // enable interrupt
    intr_on();

    // Wait for Interrupt (WFI)
    // The CPU will pause here until a timer interrupt occurs.
    // When the timer interrupt occurs -> trap_handler -> schedule -> check if there is a new process
    // If there is no new process -> schedule selects idle again -> switch_context returns here -> continue the loop
    asm volatile("wfi");
  }
}

procqueue *init_procqueue(void) {
  procqueue *q = (procqueue *)kalloc();
  if (!q)
    return NULL;
  q->head = q->tail = NULL;
  q->count = 0;
  return q;
}

void enqueue(procqueue *queue, PCB *pcb) {
  if (!queue || !pcb)
    return;
  pcb->next = NULL;
  if (queue->tail == NULL) {
    queue->head = queue->tail = pcb;
  } else {
    queue->tail->next = pcb;
    queue->tail = pcb;
  }
  queue->count++;
}

PCB *dequeue(procqueue *queue) {
  if (!queue || queue->head == NULL)
    return NULL;
  PCB *p = queue->head;
  queue->head = p->next;
  if (queue->head == NULL)
    queue->tail = NULL;
  p->next = NULL;
  queue->count--;
  return p;
}

PCB *proc_create(const char *name, uint64_t entrypoint, int prior) {
  if (!ready_queue)
    return NULL;
  // allocate PCB
  PCB *pcb = (PCB *)kalloc();
  if (!pcb)
    return NULL;
  memset(pcb, 0, sizeof(PCB));
  pcb->pid = next_pid++;
  pcb->pstat = READY;
  pcb->prior = prior;
  pcb->entrypoint = entrypoint;
  // copy name
  int i;
  for (i = 0; i < 19 && name && name[i]; i++)
    pcb->name[i] = name[i];
  pcb->name[i] = '\0';

  // allocate stack (one page)
  void *stk = kalloc();
  if (!stk) {
    kfree(pcb);
    return NULL;
  }
  pcb->stacktop = (uint64_t)stk + PAGE_SIZE;

  // initialize register state: set sepc/mepc to entrypoint and sp
  memset(&pcb->regstat, 0, sizeof(RegState));

  pcb->regstat.x1 = (uint64_t)forkret; // set return address to forkret
  pcb->regstat.sepc = entrypoint;      // switch_context will load it to mepc
  pcb->regstat.sp = pcb->stacktop;

  uint64_t mstatus_val = 0;
  mstatus_val |= (3ULL << 11);         // Set MPP to Machine Mode
  mstatus_val |= (1ULL << 7);          // Set MPIE to 1
  pcb->regstat.mstatus = mstatus_val;

  enqueue(ready_queue, pcb);

  return pcb;
}

void scheduler_init(void) {
  if (!ready_queue) {
    INFO("scheudler init...");
    ready_queue = init_procqueue();

    // === create Idle 进程 ===
    idle_proc = (PCB *)kalloc();
    if (!idle_proc)
      while (1)
        ;

    memset(idle_proc, 0, sizeof(PCB));
    idle_proc->pid = 0; // set pid Idle = 0
    idle_proc->pstat = READY;

    // process name
    char *name = "IDLE";
    for (int i = 0; i < 4; i++)
      idle_proc->name[i] = name[i];

    // allocate stack
    void *stk = kalloc();
    if (!stk)
      while (1)
        ;
    idle_proc->stacktop = (uint64_t)stk + PAGE_SIZE;

    // initialize context
    memset(&idle_proc->regstat, 0, sizeof(RegState));
    // return address
    idle_proc->regstat.x1 = (uint64_t)forkret;
    // entry
    idle_proc->regstat.sepc = (uint64_t)idle_entry;
    idle_proc->regstat.sp = idle_proc->stacktop;

    // initialize mstatus (Machine Mode, MPIE=1)
    uint64_t mstatus_val = 0;
    mstatus_val |= (3ULL << 11);
    mstatus_val |= (1ULL << 7);
    idle_proc->regstat.mstatus = mstatus_val;

    INFO("Scheduler & Idle process initialized.");
  }
}

PCB *get_current_proc(void) { return current_proc; }

void proc_exit(void) {
  intr_off();
  if (!current_proc)
    return;

  current_proc->pstat = TERMINATED;
  current_proc->next = zombie_list;
  zombie_list = current_proc;
  printk(BLUE "[proc]: \tProcess %d exited, added to zombie list." RESET "\n", current_proc->pid);

  schedule();

  while (1) {
    asm volatile("wfi");
  }
}

// free zombie memory
void zombies_free(void) {
  // turn off interrupt to avoid break in while operating link list
  intr_off();

  while (zombie_list != NULL) {
    PCB *victim = zombie_list;
    // remove from zombie link list
    zombie_list = victim->next;

    // ensure the meomory will free is not current process
    if (victim == current_proc) {
      // will not occur
      continue;
    }

    printk(BLUE "[proc]: \tReaping zombie pid=%d" RESET "\n", victim->pid);

    // 1. free stack
    // before: stacktop = stk + PAGE_SIZE
    // so the address of allocated is stacktop - PAGE_SIZE
    void *stk = (void *)(victim->stacktop - PAGE_SIZE);
    kfree(stk);
    printk(BLUE "[proc]: \tfree stack of zombie pid=%d" RESET "\n", victim->pid);

    // 2. free PCB
    kfree(victim);
    printk(BLUE "[proc]: \tfree victim" RESET "\n");
  }

  // whether to recover interrupt depends on the caller
  // this function is called by `schedual`, and `schedual` will solve interrupts
}

void schedule(void) {
  // disable interrupt
  intr_off();

  PCB *next = dequeue(ready_queue);

  // === if queue is empty, decide which process will run? ===
  if (!next) {
    // 1. If the current process is valid, running, and not the Idle process
    //    Then let it keep running (if Round Robin has no object to rotate, it runs by itself)
    if (current_proc && current_proc->pstat == RUNNING && current_proc != idle_proc) {
      next = current_proc;
    }
    // 2. In other situations:
    //    (the current process has exited, or it is currently Idle), switch to Idle
    else {
      next = idle_proc;
    }
  }

  // If we ultimately decide to continue running the current process (and both are running)
  // no switch is needed
  // Note: Optimization is possible when switching from Idle to Idle or from User to User
  // However, for the zombie cleanup logic (try_free_zombies)
  // we can still go through the switch process even for Idle->Idle
  if (next == current_proc && next->pstat == RUNNING) {
    // Still need to try to reap zombies
    // (for example, a process just exited, and now Idle is running)
    zombies_free();
    intr_on();
    return;
  }

  // --- switch context ---

  // Handle old processes
  PCB *old = current_proc;

  // if it is the first call during startup
  if (!old) {
    next->pstat = RUNNING;
    current_proc = next;
    switch_context(&boot_ctx, &next->regstat);
    intr_on();
    return;
  }

  // if the old process is RUNNING (time slice expired), put it back in the queue
  if (old->pstat == RUNNING) {
    old->pstat = READY;
    // Note: The Idle process never enters the ready_queue
    if (old != idle_proc) {
      enqueue(ready_queue, old);
    }
  }

  // If the old process is TERMINATED
  // it is already on the zombie_list
  // so we ignore it here

  next->pstat = RUNNING;
  current_proc = next;

  // switch context
  switch_context(&old->regstat, &next->regstat);

  // --- After switching back ---

  // This logic applies to all processes (including Idle):
  // Whenever there's an opportunity to get the CPU, clean up zombies along the way
  zombies_free();

  intr_on();
}
