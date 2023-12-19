#ifndef XV6_TYPES_H
#define XV6_TYPES_H

/* --- Boletín 3. Ejercicio 1. --- */
enum proc_prio {HI_PRIO = 0, NORM_PRIO = 1};

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

#ifndef NULL
#define NULL 0
#endif

#endif
