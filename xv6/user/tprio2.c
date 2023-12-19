/* --- Boletín 3. Ejercicio 2. --- */

#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  // Padre termina
  if (fork() != 0)
    exit(EXIT_SUCCESS);
  
  // Establecer prioridad normal. El shell aparecerá normalmente.
  setprio (getpid(), NORM_PRIO);

  int r = 0;
  
  for (int i = 0; i < 2000; ++i)
    for (int j = 0; j < 1000000; ++j)
      r += i + j;

  // Imprime el resultado
  printf (1, "Resultado: %d\n", r);
  
  exit(EXIT_SUCCESS);
}
