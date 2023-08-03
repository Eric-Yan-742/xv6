#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/*
 * Only be able to find data files
*/
void find(char *path, char *name)
{
    int fd;
    struct dirent de;
    struct stat st;
    char buf[512], *p;

    fd = open(path, 0);
    
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("find: path too long\n");
        exit(1);
    }

    strcpy(buf, path);
    p = buf+strlen(buf);
    if(*(p - 1) != '/') { //Check if entered dir ends with /
        *p++ = '/';
    }

    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
        if(de.inum == 0) {
            continue;
        }
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0) {
            printf("ls: cannot stat %s\n", buf);
            continue;
        }
        switch(st.type) {
            case T_FILE:
                if(strcmp(de.name, name) == 0) {
                    printf("%s\n", buf);
                }
                break;
            case T_DIR:
                if(strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0) {
                    find(buf, name);
                }
                break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Destination and Target File required!\n");
        exit(1);
    }
    int fd;
    struct stat st;
    if((fd = open(argv[1], 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", argv[1]);
        exit(1);
    }
    fstat(fd, &st);
    if(st.type != T_DIR) {
        printf("Second Argument must be a directory!\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}