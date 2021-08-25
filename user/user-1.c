//
// Created by tiziana on 23/08/21.
//

#include <sys/ipc.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "../tag_lib.h"

int main(int argc, char **argv) {
    int res;
    res = tag_get(IPC_PRIVATE, 0, 1);
    if (errno == EEXIST) {
        printf("Error tag_get: %s\n", strerror(errno));
    }
    printf("tag descriptor generated %d\n", res);

    int pid = fork();
    printf("hello %d\n", pid);

    if (pid == 0) {
        printf("hello im son\n");
        char *buff = malloc(100);
        memset(buff, 0, 100);
        res = tag_receive(1, 1, buff, 100);
        if (res < 0) {
            printf("Error tag_receive: %s\n", strerror(errno));
        }else{
            printf("message received %s\n", buff);
        }
    } else {
        char *str = "hello son\n";

        sleep(5);
        res = tag_send(1, 1, str, 12);
        if (res < 0) {
            printf("Error tag_get: %s\n", strerror(errno));
        }
        return 0;
    }
    return 0;

}

