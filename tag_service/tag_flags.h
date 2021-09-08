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
#define MESSAGE (NO+1)
#define AWAKE (MESSAGE + 1)

#define GOT_PERMISSION(permission, do_check)({ \
        /* only creator user can access to this tag and the current user correspond to him */ \
        (do_check && permission == current_uid().val)  ||                                      \
        /* all user can access to this tag */                                              \
        !do_check ||                 \
        /* root user */                             \
        current_uid().val == 0; \
})


struct msg_t {
    char *msg;
    size_t size;
};
typedef struct msg_t *msg_ptr_t;

struct rcu_util {
    unsigned long standings[2];
    int current_epoch;
    int awake[2]; // used as awake condition for the wait event queue
    struct mutex mtx; // used to have mutual exclusion between senders
};
typedef struct rcu_util *rcu_util_ptr;


struct tag_t {
    int key; // instance key
    kuid_t uid; // creator uid
    bool perm; // 1 if it is restricted to the creator user; 0 if it is public (all case)
    msg_ptr_t msg_store[LEVELS];
    wait_queue_head_t the_queue_head[LEVELS][2]; //wait event queue head
    rcu_util_ptr msg_rcu_util_list[LEVELS];
};
typedef struct tag_t *tag_ptr_t;


typedef struct tag_info_t {
    tag_ptr_t tag_ptr;
    struct rw_semaphore tag_node_rwsem;
} tag_node;

typedef tag_node *tag_node_ptr;

/**
 * @description Create a new instance associated with the key or opens an existing one by using the key.
 * This function act differently basing on the command and key combination.
 * @param key associated to a tag or IPC_PRIVATE
 * @param command Use IPC_CREAT to create a new tag instance associated to the corresponding key or to open an existing one.
 * If  IPC_CREAT | IPC_EXCL is specified and the tag instance associated to the key already exists an error is generated.
 * @param permissions 0 to grant all user access, > 0 if the access is restricted to the creator
 * @return a tag descriptor on success or an appropriate error code.
 */
int tag_get(int key, int command, int permissions);

/**
 * @description Send a message to the corresponding tag-level instance, awake all waiting threads the waits delivery ends up.
 * This function could be blocking and could be interrupted by a signal.
 * This service doesn't keep any mesage log; if nobody waits for the incoming message this is discarded.
 * @param tag tag descriptor
 * @param level message source level
 * @param buffer userspace buffer address
 * @param size buffer lenght, empty messages are anyhow allowed.
 * @return 0 on success, appropriate error code otherwise
 */
int tag_send(int tag, int level, char *buffer, size_t size);

/**
 * @description This operation blocks the caller untill an incoming message arrives from the corresponding tag-level instance.
 * The caller could be unlocked even if a signal arrives or another thread calls tag_clt with the AWAKE_ALL command.
 * @param tag tag descriptor
 * @param level message source level
 * @param buffer userspace buffer address
 * @param size buffer lenght
 * @return bytes copied on success, appropriate error code otherwise.
 */
int tag_receive(int tag, int level, char *buffer, size_t size);

/**
 * @description This operation control a tag instance by awakening operation or the by removing operation.
 * This function act differently basing on the command and key combination.
 * @param tag tag descriptor
 * @param command use IPC_RMID command to remove a tag instance, this will fail if there are readers waiting for a message on the corresponding tag.
 * IPC_RMID command can be combinating with IPC_NOWAIT command to have a nonblocking behavior.
 * Use the AWAKE_ALL command to wake up all thread waiting for a message on the corresponding tag indipendently of the level.
 * @return positive value on success, negative on failure and errno is set to the correct error code.
 */
int tag_ctl(int tag, int command);

void tag_cleanup_mem(tag_ptr_t tag);

int awake_all(int tag);

/**
 * @description Allows tag instance creation and correct initialization.
 * @param in_key associated to a tag or IPC_PRIVATE
 * @param permissions 0 to grant all user access, > 0 if the access is restricted to the creator
 * @return tag descriptor on sussess, an error code on failure
 */
int create_tag(int in_key, int permissions);

/**
 * @description Allows tag instance deletion.
 *
 * Be carefull : take write lock on tag_list[tag]->tag_node_rwsem OUTSIDE of this function and use nowait=1 to provide a nonblocking
 * behavior; if nowait is specified and the resource is not immediately available the operation abort and -EBUSY returned
 */
int remove_tag(int tag, int nowait);

#endif //SOA_PROJECT_TM_TAG_FLAGS_H
