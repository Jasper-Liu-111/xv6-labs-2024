#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
pipeline(int read_fd) __attribute__((noreturn));

void
pipeline(int read_fd) {
  int n;
  int prime;
  int number;
  int p[2];
  int pid;
  n = read(read_fd, &prime, sizeof(prime));
  if (n == sizeof(prime)) {
    fprintf(1, "prime %d\n", prime);
  } else {
    close(read_fd);
    exit(0);
  }

  if (pipe(p) < 0) {
    fprintf(2, "pipe fail\n");
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    fprintf(2, "fork fail\n");
    exit(1);
  }
  if (pid == 0) {
    close(read_fd);
    close(p[1]);
    pipeline(p[0]);
  } else {
    close(p[0]);
    while (sizeof(number) == read(read_fd, &number, sizeof(number))) {
      if (number % prime) {
        write(p[1], &number, sizeof(number));
      }
    }
    close(read_fd);
    close(p[1]);
    wait(0);
    exit(0);
  }
}

int
main(void)
{
  int p[2];
  int pid;
  int n;
  if (pipe(p) < 0) {
    fprintf(2, "pipe fail\n");
    exit(1);
  }
  pid = fork();
  if (pid < 0) {
    fprintf(2, "fork fail\n");
    exit(1);
  }
  if (pid == 0) {
    close(p[1]);
    pipeline(p[0]);
  } else {
    close(p[0]);
    for (int i = 2; i <= 280; i++) {
      n = write(p[1], &i, sizeof(i));
      if (n != sizeof(i)) {
        fprintf(2, "write fail\n");
        exit(1);
      }
    }
    close(p[1]);
    wait(0);
  }
  exit(0);
}