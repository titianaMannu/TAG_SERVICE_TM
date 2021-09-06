//
// Created by tiziana on 9/4/21.
//

#ifndef SOA_PROJECT_TM_TAG_INTERFACE_H

#include "../tag_service/tag.h"

#define SOA_PROJECT_TM_TAG_INTERFACE_H

#endif //SOA_PROJECT_TM_TAG_INTERFACE_H


struct thread_arg_t {
    int tag;
    int level;
    int msg_size;
    int command;
};

typedef struct thread_arg_t *arg_ptr_t;

void * sender(arg_ptr_t args);

void * receiver(arg_ptr_t args);

void * awaker(arg_ptr_t args);

void * remover(arg_ptr_t args);