#include <sys/ipc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../tag_lib.h"
#include "tag-interface.h"

int main(void) {
    int tag_descriptor_1;
    tag_descriptor_1 = tag_get(1, IPC_CREAT, getuid());

    if (errno == EEXIST) {
        printf("Error tag_get: %s\n", strerror(errno));
        return -1;
    }

    printf("tag descriptor generated %d\n", tag_descriptor_1);

    arg_ptr_t args_receivers_1 = malloc(sizeof(struct thread_arg_t));
    if (args_receivers_1 == NULL) {
        printf("Unable to allocate memory\n");
        return -1;
    }
    memset(args_receivers_1, 0, sizeof(struct thread_arg_t));

    args_receivers_1->tag = tag_descriptor_1;
    args_receivers_1->level = 1;
    args_receivers_1->msg_size = 100;





    /*****************************************************************************************/


    arg_ptr_t args_receivers_2 = malloc(sizeof(struct thread_arg_t));
    if (args_receivers_2 == NULL) {
        printf("Unable to allocate memory\n");
        return -1;
    }
    memset(args_receivers_2, 0, sizeof(struct thread_arg_t));

    args_receivers_2->tag = tag_descriptor_1;
    args_receivers_2->level = 2;
    args_receivers_2->msg_size = 100;

    pthread_t tid_rcv_1, tid_rcv_2, tid_rcv_3, tid_rcv_4, tid_rcv_5,  tid_snd_1, tid_snd_2, tid_awk_1;

    pthread_create(&tid_rcv_1, NULL, (void *(*)(void *)) receiver, args_receivers_1);

    pthread_create(&tid_rcv_2, NULL, (void *(*)(void *)) receiver, args_receivers_1);

    pthread_create(&tid_rcv_3, NULL, (void *(*)(void *)) receiver, args_receivers_2);

    pthread_create(&tid_rcv_4, NULL, (void *(*)(void *)) receiver, args_receivers_2);

    sleep(2);

    pthread_create(&tid_snd_2, NULL, (void *(*)(void *)) sender, args_receivers_2);

    pthread_create(&tid_awk_1, NULL, (void *(*)(void *)) awaker, args_receivers_1);

    pthread_create(&tid_rcv_5, NULL, (void *(*)(void *)) receiver, args_receivers_1);

    pthread_create(&tid_snd_1, NULL, (void *(*)(void *)) sender, args_receivers_1);


    pthread_join(tid_rcv_1, NULL);
    pthread_join(tid_rcv_2, NULL);
    pthread_join(tid_rcv_3, NULL);
    pthread_join(tid_rcv_4, NULL);
    pthread_join(tid_rcv_5, NULL);
    pthread_join(tid_snd_1, NULL);
    pthread_join(tid_snd_2, NULL);
    pthread_join(tid_awk_1, NULL);

    return 0;
}