/* --- Boletín 1. Ejercicio 3. --- */

// Test that exit and wait honor the status.
// Tiny executable so that the limit can be filling the proc table.

#include "types.h"
#include "stat.h"
#include "user.h"

#ifndef WIFEXITED
#define WIFEXITED(v) 1
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(n) (n)
#endif

#ifndef WIFSIGNALED
#define WIFSIGNALED(n) 0
#endif

#ifndef WEXITTRAP
#define WEXITTRAP(n) (n)
#endif

#define N  1000

void
forktest(void)
{
  int n, pid;
  int status;

  printf(1, "exit/wait with status test\n");

  for(n=0; n<N; n++){
    pid = fork();
    if(pid < 0)
      break;
    if(pid == 0)
      exit(n - 1/(n/40));  // Some process will fail with divide by 0
  }

  if(n == N)
  {
    printf(1, "fork claimed to work %d times!\n", N);
    exit(N);
  }

  for(; n > 0; n--)
  {
    if((pid = wait(&status)) < 0)
    {
      printf(1, "wait stopped early\n");
      exit(-1);
    }
    if (WIFEXITED (status))
      printf (1, "Exited child %d, exitcode %d\n", pid, WEXITSTATUS (status));
    else if (WIFSIGNALED(status))
      printf (1, "Exited child (failure) %d, trap %d\n", pid, WEXITTRAP (status));
  }

  if(wait(0) != -1){
    printf(1, "wait got too many\n");
    exit(-1);
  }

  printf(1, "fork test OK\n");
}

int
main(void)
{
  forktest();
  exit(0);
}
