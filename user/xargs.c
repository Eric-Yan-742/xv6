#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"


void copy_word(char *xargv[], int aIdx, int lc, char buf[])
{
    xargv[aIdx] = malloc(lc + 1);
    memset(xargv[aIdx], 0, sizeof(xargv[aIdx]));
    for(int i = 0; i < lc; i++) {
        xargv[aIdx][i] = buf[i];
    }
    xargv[aIdx][lc] = '\0';
}




int main(int argc, char *argv[])
{
    int aIdx = argc - 1, lc = 0;
    char buf[512];
    char *xargv[MAXARG];
    char letter;
    int delim_flag = 0;
    // Put all existing arguments into xargv
    for(int i = 1; i < argc; i++) {
        xargv[i - 1] = malloc(sizeof(argv[i]));
        strcpy(xargv[i - 1], argv[i]);
    }
    while(read(0, &letter, 1) > 0) { //read letter by letter
        if(letter == ' ' && delim_flag == 0) { //at the first delimiter (space)
            copy_word(xargv, aIdx, lc, buf);
            aIdx++;
            lc = 0;
            delim_flag = 1;
        } else if(letter == '\n') { // when a line ends
            if(fork() == 0) { // child process
                copy_word(xargv, aIdx, lc, buf); // copy the last word in that line
                aIdx++;
                exec(xargv[0], xargv);
            } else { //parent process
                wait(0);
                for(int i = argc - 1; i < aIdx; i++) { 
                    free(xargv[i]);  // free all dynamic memory
                }
                aIdx = argc - 1;
                lc = 0;
                delim_flag = 0;
            }
        } else if(letter != ' ') { //during a word
            buf[lc++] = letter;
            delim_flag = 0;
        }
    }
    exit(0);

}