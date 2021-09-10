/**
 * @file tag.c
 *
 * @description This file contains the code implementation for the tag_service module.
 *
 * @author Tiziana Mannucci
 *
 * @mail titianamannucci@gmail.com
 *
 * @date 13/08/2021
 *
 *
 */

#include <linux/cred.h>
#include <linux/errno.h>
#include <linux/compiler.h>
#include <linux/ipc.h>
#include <linux/filter.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/rwsem.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/mutex.h>

#include "tag_flags.h"
#include "tag.h"

extern tag_node_ptr tag_list;
extern int max_tg;
extern int *key_list;
extern int max_key;
extern unsigned msg_size;
static DEFINE_MUTEX(key_list_mtx);

/**
 * @description Create a new instance associated with the key or opens an existing one by using the key.
 * This function acts differently basing on the command and key combination.
 * @param key associated to a tag or IPC_PRIVATE
 * @param command Use IPC_CREAT to create a new tag instance associated to the corresponding key or to open an existing one.
 * If  IPC_CREAT | IPC_EXCL is specified and the tag instance associated to the key already exists an error is generated.
 * @param permissions 0 to grant all user access, > 0 if the access is restricted to the creator
 * @return a tag descriptor on success or an appropriate error code.
 * @errors
 * EINVAL: Invalid Arguments.\n
 * ENOMEM: Out of memory.\n
 * EEXIST: Tag already exists and IPC_EXCL is specified with IPC_CREAT.\n
 * EEAGAIN: Operation failed, but if you retry may success.\n
 */
int tag_get(int key, int command, int permissions) {
    int tag_descriptor;
    if (key > max_key || key < 0) {
        //not valid key
        return -EINVAL;
    }

    if (key == IPC_PRIVATE) {

        tag_descriptor = create_tag(key, permissions);
        if (tag_descriptor < 0) {
            printk(KERN_INFO "%s : Unable to create a new tag.\n", MODNAME);
            //tag creation failed
            return -ENOMEM;
        }
        /*with IPC_PRIVATE the tag is not associate to a key*/
        return tag_descriptor;
    }
    /* use xor funtions a xor (b xor a ) = a to isolate a command bit */
    if ((command ^ IPC_EXCL) == IPC_CREAT || command == IPC_CREAT) {
        /*case of IPC_CREAT | IPC_EXCL  or just IPC_CREAT */
        if (mutex_lock_interruptible(&key_list_mtx) == -EINTR) return -EINTR;

        if (key_list[key] != -1) {
            //corresponding tag already exists
            tag_descriptor = key_list[key];
            mutex_unlock(&key_list_mtx);

            /*case of IPC_CREAT | IPC_EXCL */
            if ((command ^ IPC_CREAT) == IPC_EXCL) {
                //return error because was specified IPC_EXCL
                return -EEXIST;
            }

            return tag_descriptor;

        }

        tag_descriptor = create_tag(key, permissions);
        if (tag_descriptor < 0) {
            printk(KERN_INFO "%s : Unable to create a new tag.", MODNAME);
            mutex_unlock(&key_list_mtx);
            //tag creation failed
            return -ENOMEM;
        }

        //modify key-list: insert a new tag associated to the key
        key_list[key] = tag_descriptor;
        //release lock
        mutex_unlock(&key_list_mtx);

        return tag_descriptor;

    }

    /*not valid command was specified */

    return -EINVAL;

}

/**
 * @description Send a message to the corresponding tag-level instance, awake all waiting threads then wait delivery ends up.
 * This function could be blocking and could be interrupted by a signal.
 * This service doesn't keep any message log; if nobody waits for the incoming message this is discarded.
 * @param tag tag descriptor
 * @param level message source level
 * @param buffer userspace buffer address
 * @param size buffer lenght, empty messages are anyhow allowed.
 * @return 0 on success, appropriate error code otherwise
 * @errors
 * EINVAL: Invalid Arguments.\n
 * ENOMEM: Out of memory.\n
 * ENOENT: Tag doesn't exist.\n
 * EINTR: Stopped, interrupt occured.\n
 * EPERM: Operation not permitted.\n
 * EFAULT: Message delivery fault.\n
 */
int tag_send(int tag, int level, char *buffer, size_t size) {
    tag_ptr_t my_tag;
    char *msg;
    int grace_epoch, next_epoch;
    unsigned long res;

    if (tag < 0 || tag >= max_tg || level >= LEVELS || level < 0 || buffer == NULL || size < 0 || size > msg_size) {
        /* Invalid Arguments error */
        return -EINVAL;
    }
    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag].tag_node_rwsem) == -EINTR) return -EINTR;

    my_tag = tag_list[tag].tag_ptr;
    if (my_tag != NULL) {
        /* permisson check */
        if (GOT_PERMISSION(my_tag->uid.val, my_tag->perm)) {
            /* other senders on the same tag-level exclusion */
            if (mutex_lock_interruptible(&(my_tag->msg_rcu_util_list[level]->mtx)) == -EINTR) {
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);
                return -EINTR;
            }

            if (size == 0) {
                /* nothing to copy*/
                /* release write lock on the message buffer of the corresponding level */
                mutex_unlock(&(my_tag->msg_rcu_util_list[level]->mtx));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);

                /* zero lenght messages are anyhow allowed*/
                return 0;
            }

            /*  alloc memory to copy the info */
            msg = (char *) kzalloc(size, GFP_KERNEL);
            if (msg == NULL) {
                /* release write lock on the message buffer of the corresponding level */
                mutex_unlock(&(my_tag->msg_rcu_util_list[level]->mtx));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);
                /* unable to allocate memory*/
                return -ENOMEM;
            }


            /* start to copy the message */
            res = copy_from_user(msg, buffer, size);
            asm volatile ("mfence":: : "memory");
            if (res != 0) {
                /* release write lock on the message buffer of the corresponding level */
                mutex_unlock(&(my_tag->msg_rcu_util_list[level]->mtx));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);

                kfree(msg);
                return -EFAULT;
            }

            my_tag->msg_store[level]->msg = msg;
            my_tag->msg_store[level]->size = size;

            grace_epoch = next_epoch = my_tag->msg_rcu_util_list[level]->current_epoch;
            my_tag->msg_rcu_util_list[level]->awake[grace_epoch] = MESSAGE;

            // now change epoch still under write lock
            next_epoch += 1;
            next_epoch = next_epoch % 2;
            my_tag->msg_rcu_util_list[level]->current_epoch = next_epoch;
            my_tag->msg_rcu_util_list[level]->awake[next_epoch] = NO;
            asm volatile ("mfence":: : "memory");

            /* wake up all thread waiting on the queue corresponding to the grace_epoch */
            wake_up_all(&my_tag->the_queue_head[level][grace_epoch]);

            /*wait until all readers have been consumed the message  */
            while (my_tag->msg_rcu_util_list[level]->standings[grace_epoch] > 0) schedule();

            /* here all readerers on the grace_epoch consumed the message */
            /* restore default values */
            my_tag->msg_store[level]->msg = NULL;
            my_tag->msg_store[level]->size = 0;

            /* release write lock on the message buffer of the corresponding level */
            mutex_unlock(&(my_tag->msg_rcu_util_list[level]->mtx));
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);

            kfree(msg);

            return 0;


        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);
            /* denied permission */
            return -EPERM;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag].tag_node_rwsem);
        /* tag specified not exists */
        return -ENOENT;
    }


}

/**
 * @description This operation blocks the caller untill an incoming message arrives from the corresponding tag-level instance.
 * The caller could be unlocked even if a signal arrives or another thread calls tag_clt with the AWAKE_ALL command.
 * @param tag tag descriptor
 * @param level message source level
 * @param buffer userspace buffer address
 * @param size buffer lenght
 * @return bytes copied on success, appropriate error code otherwise.
 * @errors
 * EINVAL: Invalid Arguments.\n
 * ENOBUFS: Not enough buffer space available.\n
 * ENOENT: Tag doesn't exist.\n
 * EINTR: Stopped, interrupt occured.\n
 * EPERM: Operation not permitted.\n
 * EFAULT: Message recovery fault.\n
 * ECANCELED: Operation canceled because of AWAKE notification.\n
 */
int tag_receive(int tag, int level, char *buffer, size_t size) {
    int my_epoch_msg, event_wq_ret;
    tag_ptr_t my_tag;
    unsigned long ret;

    if (tag < 0 || tag >= max_tg || level >= LEVELS || level < 0 || buffer == NULL || size < 0) {
        /* Invalid Arguments error */
        return -EINVAL;
    }

    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag].tag_node_rwsem) == -EINTR) return -EINTR;

    my_tag = tag_list[tag].tag_ptr;
    if (my_tag != NULL) {
        /* permisson check */
        if (GOT_PERMISSION(my_tag->uid.val, my_tag->perm)) {

            /*atomically add myself to the presence counter for standing readers of the current epoch  */
            my_epoch_msg = my_tag->msg_rcu_util_list[level]->current_epoch;
            __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], 1);

            /* wait event queues are used to selectively awake threads on some conditions*/
            event_wq_ret = wait_event_interruptible(my_tag->the_queue_head[level][my_epoch_msg],

                                                    my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] != NO);


            if (event_wq_ret == -ERESTARTSYS) {
                /*operation can fail also because of the delivery of a Posix signal*/
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                up_read(&tag_list[tag].tag_node_rwsem);
                return -EINTR;

            } else if (my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] == MESSAGE) {
                /* let's read the incoming message */
                if (my_tag->msg_store[level]->size > size) {
                    // provided buffer is not large enough to copy the info of the message
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                    up_read(&tag_list[tag].tag_node_rwsem);

                    return -ENOBUFS;
                }

                ret = copy_to_user(buffer, my_tag->msg_store[level]->msg, my_tag->msg_store[level]->size);
                asm volatile ("mfence":: : "memory");
                if (ret != 0) {
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                    up_read(&tag_list[tag].tag_node_rwsem);
                    /* error during the copy-- partial delivery of the message not supported */
                    return -EFAULT;
                }

                ret = my_tag->msg_store[level]->size;

                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);

                up_read(&tag_list[tag].tag_node_rwsem);

                return (int) ret;

            } else if (my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] == AWAKE) {
                /* we have been awoken by AWAKEALL routine */
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);

                up_read(&tag_list[tag].tag_node_rwsem);
                return -ECANCELED;

            }


        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);
            /* denied permission */
            return -EPERM;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag].tag_node_rwsem);
        /* tag specified not exists */
        return -ENOENT;
    }

    /*redundant ... just to be secure ! */
    up_read(&tag_list[tag].tag_node_rwsem);
    return -EFAULT;

}

/**
 * @description This operation control a tag instance by awakening operation or the by removing operation.
 * This function acts differently basing on the command and key combination.
 * @param tag tag descriptor
 * @param command use IPC_RMID command to remove a tag instance, this will fail if there are readers waiting for a message on the corresponding tag.
 * IPC_RMID command can be combinating with IPC_NOWAIT command to have a nonblocking behavior.
 * Use the AWAKE_ALL command to wake up all thread waiting for a message on the corresponding tag indipendently of the level.
 * @return non-negative value on success, negative on failure and errno is set to the correct error code.
 * @errors
 * EINVAL: Invalid Arguments.\n
 * EBUSY: Resource busy.\n
 * ENOENT: Tag doesn't exist.\n
 * EINTR: Stopped, interrupt occured.\n
 * EPERM: Operation not permitted.\n
 */
int tag_ctl(int tag, int command) {
    int ret_key;
    if (tag < 0 || tag >= max_tg) {
        /* Invalid Arguments error */
        return -EINVAL;
    }


    if (command == AWAKE_ALL) {
        return awake_all(tag);
    }
    /* use xor funtions a xor (b xor a ) = a to isolate a command bit */
    if ((command ^ IPC_NOWAIT) == IPC_RMID || command == IPC_RMID) {
        /*case of REMOVE | IPC_NOWAIT  or just REMOVE */

        // every reader and every sender currently working with this tag takes a read lock
        // we obtain the write lock when neither readers and writers are here anymore
        if (down_write_trylock(&tag_list[tag].tag_node_rwsem)) {
            // trylock is used to avoid deadlock, see documentation for detailed description.
            if ((command ^ IPC_RMID) == IPC_NOWAIT) {
                /* case of REMOVE | IPC_NOWAIT
                 * let's remove the tag */
                ret_key = remove_tag(tag, 1);
            } else {
                ret_key = remove_tag(tag, 0);
            }

            up_write(&tag_list[tag].tag_node_rwsem);
            return ret_key;

        } else {
            /*tag cannot be removed because other readers are still waiting for a message */
            return -EBUSY;
        }
    }



    // if we arrive here an invalid command was specified
    return -EINVAL;

}


void init_rcu_util(rcu_util_ptr rcu_util) {
    mutex_init(&rcu_util->mtx);
    rcu_util->standings[0] = 0;
    rcu_util->standings[1] = 0;
    rcu_util->awake[0] = NO;
    rcu_util->awake[1] = NO;
    rcu_util->current_epoch = 0;
}

/**
 * @description Allows tag instance creation and correct initialization.
 * @param in_key associated to a tag or IPC_PRIVATE
 * @param permissions 0 to grant all user access, > 0 if the access is restricted to the creator
 * @return tag descriptor on sussess, an error code on failure
 */
int create_tag(int in_key, int permissions) {
    rcu_util_ptr new_msg_rcu;
    int i, j;
    tag_ptr_t new_tag;
    for (i = 0; i < max_tg; i++) {
        if (down_write_trylock(&tag_list[i].tag_node_rwsem)) {
            //succesfull , lock acquired

            if (tag_list[i].tag_ptr == NULL) {
                new_tag = kzalloc(sizeof(struct tag_t), GFP_KERNEL);
                if (new_tag == NULL) {
                    //unable to allocate, release lock and return error
                    up_write(&tag_list[i].tag_node_rwsem);
                    return -ENOMEM;
                }


                new_tag->key = in_key;
                new_tag->uid.val = current_uid().val;
                if (permissions > 0) new_tag->perm = true;
                else new_tag->perm = false;


                for (j = 0; j < LEVELS; j++) {

                    msg_ptr_t new_msg_str = kzalloc(sizeof(struct msg_t), GFP_KERNEL);
                    if (new_msg_str == NULL) {
                        tag_list[i].tag_ptr = NULL;
                        up_write(&tag_list[i].tag_node_rwsem);
                        tag_cleanup_mem(new_tag);
                        return -ENOMEM;

                    }
                    //message buffer initialization
                    new_msg_str->size = 0;
                    new_msg_str->msg = NULL;
                    new_tag->msg_store[j] = new_msg_str;

                    new_msg_rcu = kzalloc(sizeof(struct rcu_util), GFP_KERNEL);
                    if (new_msg_rcu == NULL) {
                        tag_list[i].tag_ptr = NULL;
                        up_write(&tag_list[i].tag_node_rwsem);
                        tag_cleanup_mem(new_tag);
                        return -ENOMEM;
                    }
                    //rcu util initialization
                    init_rcu_util(new_msg_rcu);
                    new_tag->msg_rcu_util_list[j] = new_msg_rcu;

                    //wait event queues initialization
                    init_waitqueue_head(&new_tag->the_queue_head[j][0]);
                    init_waitqueue_head(&new_tag->the_queue_head[j][1]);
                }

                tag_list[i].tag_ptr = new_tag;
                asm volatile ("sfence":: : "memory");

                up_write(&tag_list[i].tag_node_rwsem);
                // return a tag descriptor
                return i;


            } else {

                up_write(&tag_list[i].tag_node_rwsem);
                continue;

            }

        }

    }

    //research failed ... there aren't free tags to use
    //if we can't get write lock on a tag it means that someone else is doing something with that tag
    //so it isn't free and you cannot insert a new one.
    return -EAGAIN;

}


void tag_cleanup_mem(tag_ptr_t tag) {
    int i;
    if (tag == NULL) return;
    for (i = 0; i < LEVELS; i++) {
        if (tag->msg_store[i] != NULL) kfree(tag->msg_store[i]);
        if (tag->msg_rcu_util_list[i] != NULL) kfree(tag->msg_rcu_util_list[i]);
    }

    kfree(tag);
}

/**
 * @description Allows tag instance deletion.
 *
 * Be carefull : take write lock on tag_list[tag]->tag_node_rwsem OUTSIDE of this function and use nowait=1 to provide a nonblocking
 * behavior; if nowait is specified and the resource is not immediately available the operation abort and -EBUSY returned
 */
int remove_tag(int tag, int nowait) {
    int ret_key;
    tag_ptr_t my_tag = tag_list[tag].tag_ptr;
    if (my_tag != NULL) {
        ret_key = my_tag->key;

        if (GOT_PERMISSION(my_tag->uid.val, my_tag->perm)) {
            /*first of all remove the key; this way the tag cannot be invoked anymore */
            if (ret_key != IPC_PRIVATE) {
                /*use nowait is IPC_NOWAIT is specified*/
                if (nowait) {
                    if (mutex_trylock(&key_list_mtx)) {
                        key_list[ret_key] = -1;
                        mutex_unlock(&key_list_mtx);
                    } else {
                        return -EBUSY;
                    }

                } else {
                    if (mutex_lock_interruptible(&key_list_mtx) == -EINTR) return -EINTR;
                    key_list[ret_key] = -1;
                    mutex_unlock(&key_list_mtx);
                }
            }
            /* delete the tag from the tag_list */
            tag_list[tag].tag_ptr = NULL;
            asm volatile ("mfence");
            /*cleanup memory previously allocated*/
            tag_cleanup_mem(my_tag);

            return ret_key;

        } else {
            /*permission denided case*/
            return -EPERM;
        }
    } else {
        /* this tag is not present */
        return -ENOENT;
    }
}

/**
 * @description Awakes all thread awaiting for a message on the corresponding tag indipendently of the level.
 * Conceptually acts as a sender but an awake notification is sent instead of a message.
 * @param tag tag descriptor
 * @return 0 on success, error code on failure.
 */
int awake_all(int tag) {
    int grace_epoch, next_epoch, level;
    tag_ptr_t my_tag;
    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag].tag_node_rwsem) == -EINTR) return -EINTR;

    my_tag = tag_list[tag].tag_ptr;
    if (my_tag != NULL) {

        if (GOT_PERMISSION(my_tag->uid.val, my_tag->perm)) {

            for (level = 0; level < LEVELS; level++) {
                /*
                 * Use trylock because if it is not immediately acquired it means that a sender is currently there
                 * to awake this level with a message or it means that another awaker is doing his job on the current epoch.
                 * this lock is acquired to protect against concurrent threads executing awakers/writers.
                 */
                if (mutex_trylock(&my_tag->msg_rcu_util_list[level]->mtx)) {

                    grace_epoch = next_epoch = my_tag->msg_rcu_util_list[level]->current_epoch;
                    my_tag->msg_rcu_util_list[level]->awake[grace_epoch] = AWAKE;

                    // now change epoch still under write lock
                    next_epoch += 1;
                    next_epoch = next_epoch % 2;
                    my_tag->msg_rcu_util_list[level]->current_epoch = next_epoch;
                    /* all the following threads belong to the new epoch and won't be awoken*/
                    my_tag->msg_rcu_util_list[level]->awake[next_epoch] = NO;
                    asm volatile ("mfence":: : "memory");
                    /* wake up all thread waiting on the queue corresponding to the grace_epoch */
                    wake_up_all(&my_tag->the_queue_head[level][grace_epoch]);
                    /*wait until all readers have been consumed the awake notification */
                    while (my_tag->msg_rcu_util_list[level]->standings[grace_epoch] > 0) schedule();

                    /* release locks previously aquired */
                    mutex_unlock(&(my_tag->msg_rcu_util_list[level]->mtx));

                }


            }
            up_read(&tag_list[tag].tag_node_rwsem);
            return 0;

        } else {
            /*permission denided case*/
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);
            return -EPERM;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag].tag_node_rwsem);
        /* tag specified not exists */
        return -ENOENT;
    }


}
