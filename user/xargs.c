#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(2, "xargs argc error\n");
        exit(1);
    }
    char *args[MAXARG];
    memset(args, 0, sizeof(char*) * MAXARG);
    int i;
    for (i = 1; i < argc; i++)
    {
        args[i - 1] = (char*)malloc(100);
        memset(args[i - 1], 0, 100);
        strcpy(args[i - 1], argv[i]);
    }
    char arg[100] = {0};
    char *pc = arg;
    char c;
    while (read(0, &c, 1) == 1)
    {
        if (c == '\n')
        {
            if(fork() == 0)
            {
                args[i - 1] = (char*)malloc(100);
                memset(args[i - 1], 0, 100);
                strcpy(args[i - 1], arg);
                exec(args[0], args);
            }
            else
            {
                wait(0);
                memset(arg, 0, 100);
                pc = arg;
            }
        }
        else
        {
            *pc++ = c;  
        }
    }
    return 0;
}