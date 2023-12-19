#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

/* --- Boletín 1. Ejercicio 3. --- */
int
sys_exit(void)
{
  int status;

  /* Recuperamos el estatus pasado como argumento a exit(...). */
  if (argint(0, &status) < 0)
    return -1;

  /* Ponemos el estado de salida en los 8 bits más significativos de los primeros 16 del entero. */
  status = status << 8;

  /* Llamada a exit con el nuevo estado de salida. */
  exit(status);
  return 0;  // not reached
}

/* --- Boletín 1. Ejercicio 3. --- */
int
sys_wait(void)
{
  int *status;

  /* Recuperamos el puntero al entero donde almacenar el estado de salida del proceso. */
  if (argptr(0, (void *)&status, sizeof(int)) < 0)
    return -1;

  /* Llamada a wait con el puntero donde almacenar el estado de salida. */
  return wait(status);
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

/* --- Boletín 2. Ejercicio 1. --- */
int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  /* Guardamos el tamaño antiguo. */ 
  addr = myproc()->sz;
  
  /* Si no se pide decrementar la memoria simplemente aumentamos la memoria del proceso. */
  if (n >= 0)
  {
    /* Si la memoria pedida llega hasta el kernel se debe fallar. */
    if (myproc()->sz + n >= KERNBASE)
      return -1;
    myproc()->sz += n;
  }
  
  /* --- Boletín 2. Ejercicio 2. --- */
  /* Si se decrementa la memoria llamamos a growproc(n). No hay que decrementar manualmente el tamaño, ya lo hace growproc().*/
  else if(growproc(n) < 0)
    return -1;
  
  /* Se devuelve el tamaño antíguo de la memoria. */
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/* --- Boletín 1. Ejercicio 1. --- */
int
sys_date(void)
{
  struct rtcdate *r;

  /* Sacamos el puntero a rtcdate de la pila. */
  if (argptr(0, (void *)&r, sizeof(struct rtcdate)) < 0)
    return -1;

  /* Si se obtiene un puntero NULL debe fallar. */
  if (r == NULL)
    return -1;
  
  /* Extraemos el tiempo actual.*/
  cmostime(r);

  return 0;
}

/* --- Boletín 3. Ejercicio 2. --- */
int
sys_getprio(void)
{
  int pid;

  /* Sacamos el pid del proceso del que se desea obtener la prioridad. */
  if (argint(0, &pid) < 0)
    return -1;

  return getprio(pid);
}

/* --- Boletín 3. Ejercicio 2. --- */
int
sys_setprio(void)
{
  int pid;
  enum proc_prio prio;

  if (argint(0, &pid) < 0)
    return -1;

  if (argint(1, (int *)&prio) < 0)
    return -1;

  return setprio(pid, prio);
}