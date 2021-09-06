//
// Created by tiziana on 9/4/21.
//

#include <string.h>
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "../tag_lib.h"
#include "tag-interface.h"


void *sender(arg_ptr_t args) {
    int res;
    size_t max_len = args->msg_size;
    char *buffer = malloc(max_len);
    if (buffer == NULL) {
        printf("unable to allocate memory\n");
        pthread_exit(NULL);
    }
    memset(buffer, 0, max_len);

    snprintf(buffer, max_len, "sender tid=%ld Hello receiver with  tag=%d and level=%d", syscall(SYS_gettid),
             args->tag,
             args->level);


    printf("sender tid=%ld, i'm going to send a message to tag= %d and level=%d my tic= %ld\n", syscall(SYS_gettid),
           args->tag,
           args->level,
           clock());

    res = tag_send(args->tag, args->level, buffer, max_len);
    if (res < 0) {
        free(buffer);
        printf("tid= %ld Error tag_send: %s\n", syscall(SYS_gettid),strerror(errno));
        pthread_exit(NULL);
    }


    free(buffer);
    pthread_exit(NULL);

}


void *receiver(arg_ptr_t args) {
    int res;
    char *buffer = malloc((sizeof(char)) * args->msg_size);
    if (buffer == NULL) {
        printf("unable to allocate memory\n");
        pthread_exit(NULL);
    }
    memset(buffer, 0, args->msg_size);

    printf("tid = %ld ready to receive my tic=%ld\n",  syscall(SYS_gettid), clock());
    res = tag_receive(args->tag, args->level, buffer, args->msg_size);
    if (res < 0) {
        printf("tid=%ld Error tag_receive: %s\n", syscall(SYS_gettid), strerror(errno));
        pthread_exit(NULL);
    }

    printf("receiver tid=%ld : message received={%s} size=%d\n", syscall(SYS_gettid), buffer, res);
    free(buffer);
    pthread_exit(NULL);
}

void *awaker(arg_ptr_t args) {
    int res;
    printf("tid = %ld awaker init: tic=%ld\n",syscall(SYS_gettid), clock());
    res = tag_ctl(args->tag, AWAKE_ALL);
    if (res < 0) {
        printf("Awaker  - Error tag_ctl: %s\n", strerror(errno));
        pthread_exit(NULL);
    }

    printf("Awaker - tid=%ld completed ...\n", syscall(SYS_gettid));

}

void *remover(arg_ptr_t args) {
    int res;
    printf("Remover tid=%ld tic=%ld\n", syscall(SYS_gettid), clock());
    res = tag_ctl(args->tag, args->command);
    if (res < 0) {
        printf("Remover  - Error tag_ctl: %s\n", strerror(errno));
        pthread_exit(NULL);
    }

    printf("Remover - tid=%ld completed ...\n", syscall(SYS_gettid));
    pthread_exit(NULL);
}