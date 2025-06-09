#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  // your code here.  you should write the secret to fd 2 using write
  // (e.g., write(2, secret, 8)
  char secret[32] = {0};
  while (1) {
    char *start = sbrk(PGSIZE*32);
    if ((uint64)start == (uint64)-1) {
      printf("sbrk failed\n");
      exit(1);
    }
    char *end = start;
    for (int i = 0; i < PGSIZE*32 - 50; i++, end++) {
      // compare full string
      if (memcmp(end, "my very very very secret pw is:   ", 31) == 0) {
        printf("%s\n", end);
        // Found the secret
        for (int j = 0; j < 8; j++) {
          secret[j] = *(end + 32 + j);
        }
        printf("Found the secret! %s\n", secret);
        write(2, secret, 8);
        exit(0);
      }

    }
  }
}
