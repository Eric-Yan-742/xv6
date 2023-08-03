#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    char byte = 'A';
    
    if(fork() == 0) { //child process
        close(p1[1]);
        read(p1[0], &byte, 1);
        close(p1[0]);
        fprintf(1, "%d: received ping\n", getpid());

        close(p2[0]);
        write(p2[1], &byte, 1);
        close(p2[1]);

    } else { //parent process
        close(p1[0]);
        write(p1[1], &byte, 1);
        close(p1[1]);

        close(p2[1]);
        read(p2[0], &byte, 1);
        close(p2[0]);
        fprintf(1, "%d: received pong\n", getpid());
    }
    exit(0);
}
