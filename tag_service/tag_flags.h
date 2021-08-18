//
// Created by tiziana on 16/08/21.
//

#ifndef SOA_PROJECT_TM_TAG_FLAGS_H


#define SOA_PROJECT_TM_TAG_FLAGS_H

#include <linux/rwsem.h>
#include <linux/uidgid.h>


#define LEVELS 32
#define MAX_TAG 256
#define MAX_KEY 256
#define MSG_LEN 4096

#define MODNAME "TAG-SERVICE"

struct msg_t{
    char * msg;
    size_t size;
};
typedef struct msg_t *msg_ptr_t;

struct rcu_util{
    unsigned long presence_counter[2];
    unsigned char current_epoch;
    unsigned char sync[2];
    struct rw_semaphore _lock;
};
typedef struct rcu_util *rcu_util_ptr;


struct tag_t {
        int key;
        kuid_t uid;
        msg_ptr_t msg_store[LEVELS];
        bool perm;
        wait_queue_head_t sync_conditional[LEVELS][2];
        rcu_util_ptr msg_rcu_util_list[LEVELS];
        rcu_util_ptr awake_rcu_util_list[LEVELS];

};


typedef struct tag_t *tag_ptr_t;


typedef struct tag_info_t {
    struct tag_t  *ptr;                      // Pointer to the corresponding instance.
    struct rw_semaphore tag_node_rwsem;   // For receivers as readers.
} tag_node;

typedef tag_node *tag_node_ptr;


#endif //SOA_PROJECT_TM_TAG_FLAGS_H
