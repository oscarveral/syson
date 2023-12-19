/* --- Boletín 2. Ejercicios 1 y 2. --- */

#include "types.h"
#include "user.h"

int i = 1;

void test1()
{
  char* a = sbrk (0);

  printf (1, "Debe fallar ahora:\n");
  *(a+1) = 1;  // Debe fallar
}

void test2()
{
  // Página de guarda:
  printf (1, "Si no fallo antes (mal), ahora tambien debe fallar:\n");
  char* a = (char*)((int)&i + 4095);
  printf (1, "%d\n", a);
  *a = 1;
}

void test3()
{
  // Acceder al núcleo
  printf (1, "Si no fallo antes (mal), ahora tambien debe fallar:\n");
  char* a = (char*)0x80000001;
  *(a+1) = 1;  // Debe fallar (si lo anterior no ha fallado)
}


int
main(int argc, char *argv[])
{
  printf (1, "Este programa primero intenta acceder mas alla de sz.\n");

  // Más allá de sz
  //test1();

  // Guarda
  //test2();

  // Núcleo
  test3();

  exit (0);
}
