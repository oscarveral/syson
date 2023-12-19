#include <syscall.h>
#include <traps.h>

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)

/* --- Boletín 1. Ejercicio 1. --- */
SYSCALL(date)

/* --- Boletín 1. Ejercicio 2. --- */
SYSCALL(dup2)

/* --- Boletín 3. Ejercicio 2. --- */
SYSCALL(getprio)
SYSCALL(setprio)