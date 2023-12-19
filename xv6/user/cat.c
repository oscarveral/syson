#include "types.h"
#include "stat.h"
#include "user.h"

char buf[512];

void
cat(int fd)
{
  int n;

  while((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      printf(1, "cat: write error\n");
      exit(EXIT_FAILURE);
    }
  }
  if(n < 0){
    printf(1, "cat: read error\n");
    exit(EXIT_FAILURE);
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if(argc <= 1){
    cat(0);
    exit(EXIT_SUCCESS);
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "cat: cannot open %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }
    cat(fd);
    close(fd);
  }
  exit(EXIT_SUCCESS);
}
