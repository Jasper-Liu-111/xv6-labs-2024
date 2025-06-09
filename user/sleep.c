#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
      fprintf(2, "Usage: sleep <int>\n");
      exit(1);
  }
  i = atoi(argv[1]);
  fprintf(1, "%d\n", i);
  sleep(i);
  exit(0);
}
