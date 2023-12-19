/* --- Boletín 3. Ejercicio 2. --- */

#include "types.h"
#include "user.h"

void
do_calc (char* nombre)
{
  int r = 0;

  for (int i = 0; i < 2000; ++i)
    for (int j = 0; j < 1000000; ++j)
      r += i + j;

  // Imprime el resultado
  printf (1, "%s: %d\n", nombre, r);
}


int
main(int argc, char *argv[])
{
  // El proceso se inicia en baja prioridad.
  // Genera otro proceso hijo que a su vez genera dos
  if (fork() == 0)
  {
    fork();  // Ambos ejecutan:
    do_calc("Low");
    exit(EXIT_SUCCESS);
  }

  // Establecer máxima prioridad. Debe hacer que el shell ni aparezca hasta
  // que termine
  setprio (getpid(), HI_PRIO);

  fork();  // Ambos ejecutan:
  do_calc("Hi");
  exit(EXIT_SUCCESS);
}
