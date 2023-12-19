#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

/* --- Boletín 2. Ejercicio 1. --- */
/* Declaración de mappages como función externa. */
extern int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
/* --- Boletín 2. Ejercicio 2. --- */
/* Declaración de walkpgdir como función externa. */
extern pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      
      /* --- Boletín 1. Ejercicio 3. --- */
      /* 
         Cuando se sale por un trap el estado de salida se almacena en los 8 bits menos significativos.
         Para que sea reconocible por WIFSIGNALED(...) esos 8 bits no deben valer 0, pero como existe una
         trap con valor 0, para poder reconocerla simplemente sumamos 1 a todos los valores de trapno que
         pasemos como código de salida y luego la macro WEXITTRAP restará 1 para obtener el valor original.
      */
      exit(tf->trapno + 1);
    
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      
      /* --- Boletín 1. Ejercicio 3. --- */
      exit(tf->trapno + 1);
    
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  /* --- Boletín 2. Ejercicio 1. --- */
  /* Excepción de fallo de página. */
  case T_PGFLT:

    uint fltaddr = rcr2();

    /* Si la dirección de fallo de página está por encima del tamaño de la memoria es un fallo catastrófico. Hay que matar el proceso. */
    if (fltaddr >= myproc()->sz)
    {
      if (fltaddr >= KERNBASE)
        cprintf("Page fault on addr: 0x%x outside user space memory\n", fltaddr);
      else
        cprintf("Page fault on addr: 0x%x outside assigned process memory\n", fltaddr);
      myproc()->killed = 1;
      break;
    }

    /* --- Boletín 2. Ejercicio 2. --- */
    /* su entrada en la tabla de páginas. */
    uint fltpage = PGROUNDDOWN(fltaddr);
    pde_t * pgfltpde = walkpgdir(myproc()->pgdir, (void *)fltpage, 0);
 
    /* Comprobamos si la página estaba reservada y en ese caso sí estaba presente. */
    if (pgfltpde && *pgfltpde & PTE_P)
    {
      /* Si el bit de user no está activado en error el fallo es del kernel sobre una página presente y el sistema no puede seguir. */
      if (!(tf->err & PTE_U))
        panic("kernel had a page fault");

      /* Si no es una página de usuario se falla. */
      if (!(*pgfltpde & PTE_U))
        cprintf("Page fault on addr: 0x%x. Tryed to access protected memory.\n", fltaddr);
        
      /* Si la página es de usuario el fallo ha sido de lectura o escritura. */
      else
      {
        if (tf->err & PTE_W)
          cprintf("Page fault by error on write access on 0x%x", fltaddr);
        else
          cprintf("Page fault by error on read access on 0x%x", fltaddr);
      }
      myproc()->killed = 1;
      break;
    }

    /* Reservamos memoria para la página pedida y la inicializamos con 0s. */
    char *mem;
    if ((mem = kalloc()) == 0)
    {
      cprintf("error by kalloc on page fault. out of memory\n");
      myproc()->killed = 1;
      break;
    }
    memset(mem, 0, PGSIZE);

    /* Mapeamos la memoria reservada a la página correspondiente con la dirección de fallo de página. */
    if (mappages(myproc()->pgdir, (char*)fltpage, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0)
    {
      cprintf("error by mappages on page fault, cant cant map physical page on page table\n");
      myproc()->killed = 1;
    }

    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)

    /* --- Boletín 1. Ejercicio 3. --- */
    exit(tf->trapno + 1);

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    
    /* --- Boletín 1. Ejercicio 3. --- */
    exit(tf->trapno + 1);
}
