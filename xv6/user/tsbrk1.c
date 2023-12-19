/* --- Boletín 2. Ejercicios 1 y 2. --- */

#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char* a = sbrk (15000);

  a[500] = 1;

  if ((uint)a + 15000 != (uint) sbrk (-15000))
  {
    printf (1, "sbrk() con número positivo falló.\n");
    exit(1);
  }

  if (a != sbrk (0))
  {
    printf (1, "sbrk() con cero falló.\n");
    exit(2);
  }

  if (a != sbrk (15000))
  {
    printf (1, "sbrk() negativo falló.\n");
    exit(3);
  }

  printf (1, "Debe imprimir 1: %d.\n", ++a[500]);

  exit(0);
}
