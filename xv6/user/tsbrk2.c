/* --- Boletín 2. Ejercicios 1 y 2. --- */

#include "types.h"
#include "user.h"

char a[4096] = {0};

int recursive(int v)
{
  printf (1, ".");
  volatile int q = v;
  if (q > 0)
    return recursive (q+1)+recursive (q+2);
  return 0;
}


int
main(int argc, char *argv[])
{
  int i = 1;

  // Llamar recursivamente a recursive
  printf (1, ": %d\n", recursive (i));

  exit(0);
}
