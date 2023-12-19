#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];

  /* --- Boletín 3. Ejercicio 1. --- */
  struct proc *queue[NUM_PRIO];

} ptable;

/* --- Boletín 3. Ejercicio 1. --- */
/* Función que mete el proceso p en la cola correspondiente según su prioridad. Requisito: Debe tenerse el cerrojo sobre la tabla de procesos y la prioridad establecida.*/
void enqueue(struct proc *p)
{
  /* Pánico si el proceso ya estaba en una cola. */
  if (p->previous != NULL && p->next != NULL)
    panic("enqueue: can't push process that is already in a queue\n");
  /* Pánico si el proceso no está bien sacado de una cola. */
  if ((p->previous == NULL && p->next != NULL) || 
      (p->previous != NULL && p->next == NULL))
    panic("enqueue: corrupted process priority queue data\n");

  /* Si la cola correspondiente está vacía hay que insertarlo como el primero apuntandose a sí mismo. */
  if (ptable.queue[p->priority] == NULL)
  {
    ptable.queue[p->priority] = p;
    p->previous = p;
    p->next = p;
    return;
  }

  /* En otro caso hay que insertarlo al final de la cola. */
  ptable.queue[p->priority]->previous->next = p;
  p->previous = ptable.queue[p->priority]->previous;
  ptable.queue[p->priority]->previous = p;
  p->next = ptable.queue[p->priority];
  return;
}

/* --- Boletín 3. Ejercicio 1. --- */
/* Función que saca el proceso p de la cola correspondiente según su prioridad. Requisito: Debe tenerse el cerrojo sobre la tabla de procesos y la prioridad establecida.*/
void dequeue(struct proc *p)
{
  /* Pánico si se intenta sacar un proceso de una cola vacía. */
  if (ptable.queue[p->priority] == NULL)
    panic("dequeue: can't pop from a queue that was empty\n");
  /* Pánico si se intenta sacar de la cola un proceso que no está en ninguna. */
  if (p->previous == NULL && p->next == NULL)
        panic("dequeue: can't pop process that is not in a queue\n");
  /* Pánico el proceso no está bien insertado en una cola cuando se den los siguientes casos. */
  if ((p->previous == NULL && p->next != NULL) || 
      (p->previous != NULL && p->next == NULL) || 
      (p->previous == p    && p->next != p)    || 
      (p->previous != p    && p->next == p))
    panic("dequeue: corrupted process priority queue data\n");
  
  /* Si el proceso es el único de la cola hay que vaciarla. */
  if (p->previous == p && p->next == p)
  {
    p->previous = NULL;
    p->next = NULL;
    ptable.queue[p->priority] = NULL;
    return;
  }

  /* En otro caso el proceso está en una cola con otros y hay que retirarlo. */
  p->next->previous = p->previous;
  p->previous->next = p->next;

  /* Si el proceso a eliminar era el primero de la lista hay que actualizar para que el próximo sea el primero. */
  if (ptable.queue[p->priority] == p)
    ptable.queue[p->priority] = p->next;

  p->previous = NULL;
  p->next = NULL;
  return;
}

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  /* --- Boletín 3. Ejercicio 1. --- */
  /* El proceso inicial tiene prioridad normal. */
  p->priority = NORM_PRIO;
  p->previous = NULL;
  p->next = NULL;
  /* El proceso está listo y lo insertamos en su cola. */
  enqueue(p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  lcr3(V2P(curproc->pgdir));  // Invalidate TLB.
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  /* --- Boletín 3. Ejercicio 1. --- */
  /* Cuando un proceso hace un fork hereda la prioridad del padre. */
  np->priority = np->parent->priority;
  /* El proceso está listo y lo insertamos en su cola. */
  enqueue(np);

  release(&ptable.lock);

  return pid;
}

/* --- Boletín 1. Ejercicio 3. --- */
// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(int status)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Optimize by removing user part
  deallocuvm(curproc->pgdir, KERNBASE, 0);

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;

  /* --- Boletín 1. Ejercicio 3. --- */
  curproc->status = status;

  sched();
  panic("zombie exit");
}

/* --- Boletín 1. Ejercicio 3. --- */
// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(int *status)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir, 0); // User zone deleted before
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;

        /* --- Boletín 3. Ejercicio 1. --- */
        /* Al liberarse el proceso limpiamos su prioridad, y procesos siguiente y previo. */
        /* Esto implica que todas las entradas para procesos no usados tendrán prioridad alta y siempre tendrán los punteros a procesos a NULL. */
        p->priority = 0;
        p->next = NULL;
        p->previous = NULL;

        /* --- Boletín 1. Ejercicio 3. --- */
        /* Si el puntero no es nulo podemos copiar el estado de salida al puntero dado. */
        if (status != NULL)
          *status = p->status;

        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
  
    /* --- Boletín 3. Ejercicio 1. --- */
    /* El nuevo loop del scheduler simplemente seleccionará el próximo proceso de la lista que no esté vacía según prioridad. */
    if ((p = ptable.queue[HI_PRIO]) == NULL)
      p = ptable.queue[NORM_PRIO];

    if (p != NULL)
    {
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.

      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      /* Una vez se pone en ejecución el proceso se saca de la cola. */
      dequeue(p);
      
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }

    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;

  /* --- Boletín 3. Ejercicio 1. --- */
  /* El proceso está listo para ser ejecutado y hay que ponerlo en cola. */
  enqueue(myproc());
  
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
    {
      p->state = RUNNABLE;
      
      /* --- Boletín 3. Ejercicio 1. --- */
      /* El proceso está listo para ser ejecutado por lo que lo ponemos en cola. */
      enqueue(p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
      {
        p->state = RUNNABLE;
        /* --- Boletín 3. Ejercicio 1. --- */
        /* El proceso está listo para ejecutarse, lo ponemos en cola. */
        enqueue(p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

/* --- Boletín 3. Ejercicio 2. --- */
/* Función que retorna la prioridad del proceso con el pid dado. */
enum proc_prio
getprio(int pid)
{
  struct proc *p;

  /* Para recorrer la tabla de procesos necesitamos su cerrojo. */
  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
    if (p->pid == pid && p->state != UNUSED)
    {
      release(&ptable.lock);
      return p->priority;
    }

  release(&ptable.lock);

  /* Si no se encuentra el pid se retorna -1 como error. */
  return -1;
}

/* --- Boletín 3. Ejercicio 2. --- */
int
setprio(int pid, enum proc_prio prio)
{
  if (prio != HI_PRIO && prio != NORM_PRIO)
    return -1;

  struct proc *p;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
    if (p->pid == pid && p->state != UNUSED)
    {
      if (p->priority != prio)
      {
        if (p->state == RUNNABLE)
        {
          dequeue(p);
          p->priority = prio;
          enqueue(p);
        }
        else
          p->priority = prio;
      }
      release(&ptable.lock);
      return 0;
    }

  release(&ptable.lock);

  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
