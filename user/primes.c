#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int left_read)
{
    int num, p;
    if(read(left_read, &p, 4) == 0) {
        close(left_read);
        exit(0);
    }
    printf("prime %d\n", p);
    int right[2];
    pipe(right);

    if(fork() == 0) {
        close(right[1]);
        close(left_read);
        sieve(right[0]);
    } else {
        close(right[0]);
        while(read(left_read, &num, 4) > 0) {
            if(num % p != 0)
                write(right[1], &num, 4);
        }
        close(left_read);
        close(right[1]);
        wait(0);
        exit(0);
    }
}

int
main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    if(fork() == 0) {
        close(p[1]);
        sieve(p[0]);
    } else {
        close(p[0]);
        for(int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    return 0;
}
