#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int pid;
  int pipe1[2];
  int pipe2[2];
  int n;
  char byte;
  if(pipe(pipe1) < 0 || pipe(pipe2) < 0){
    printf("pipe() failed\n");
    exit(1);
  }

  pid = fork();
  if(pid < 0){
    printf("fork() failed\n");
    exit(1);
  }
  if(pid == 0){
    n = read(pipe1[0], &byte, 1);
    close(pipe1[0]);
    if(n != 1){
      printf("read() failed\n");
      exit(1);
    }
    fprintf(1, "%d: received ping\n", getpid());
    n = write(pipe2[1], &byte, 1);
    close(pipe2[1]);
    if(n != 1){
      printf("write() failed\n");
      exit(1);
    }
  } else {
    n = write(pipe1[1], &byte, 1);
    close(pipe1[1]);
    if(n != 1){
      printf("write() failed\n");
      exit(1);
    }
    n = read(pipe2[0], &byte, 1);
    close(pipe2[0]);
    if (n != 1){
      printf("read() failed\n");
      exit(1);
    }
    fprintf(1, "%d: received pong\n", getpid());
  }
  exit(0);
}
