//
// Created by tiziana on 16/08/21.
//

#ifndef SOA_PROJECT_TM_TAG_FLAGS_H

#include <stdbool.h>
#include <linux/rwsem.h>
#include <linux/uidgid.h>
#include "tag.h"

#define SOA_PROJECT_TM_TAG_FLAGS_H


#define NO (0)
#define YES (NO+1)


struct msg_t {
    char *msg;
    size_t size;
};
typedef struct msg_t *msg_ptr_t;

struct rcu_util {
    unsigned long standings[2];
    int current_epoch;
    int awake[2];
    struct rw_semaphore sem;
};
typedef struct rcu_util *rcu_util_ptr;


struct tag_t {
    int key;
    kuid_t uid;
    bool perm; // 1 if it is restricted to the creator user; 0 if it is public (all case)
    msg_ptr_t msg_store[LEVELS];
    wait_queue_head_t the_queue_head[LEVELS][2];
    rcu_util_ptr msg_rcu_util_list[LEVELS];
    rcu_util_ptr awake_rcu_util;

};


typedef struct tag_t *tag_ptr_t;


typedef struct tag_info_t {
    struct tag_t *tag_ptr;
    struct rw_semaphore tag_node_rwsem;
} tag_node;

typedef tag_node *tag_node_ptr;



int tag_get(int key, int command, int permissions);

int tag_send(int tag, int level, char *buffer, size_t size);

int tag_receive(int tag, int level, char *buffer, size_t size);

int tag_ctl(int tag, int command);

void tag_cleanup_mem(tag_ptr_t tag);

#endif //SOA_PROJECT_TM_TAG_FLAGS_H
