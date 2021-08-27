/**
 * @file tag.c
 * @brief todo insert a detailed description
 *
 * @author Tiziana mannucci
 *
 * @mail titianamannucci@gmail.com
 *
 * @date 13/08/2021
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/rwsem.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/cred.h>
#include <linux/errno.h>
#include <linux/compiler.h>
#include <linux/ipc.h>
#include <linux/filter.h>

#include "tag_flags.h"
#include "tag.h"

extern tag_node_ptr tag_list;
extern int max_tg;
extern int *key_list;
extern int max_key;
extern struct rw_semaphore key_list_sem;

int create_tag(int in_key, int permissions);

int remove_tag(int tag, int nowait);

int tag_get(int key, int command, int permissions) {
    int tag_descriptor;
    if (key > max_key || key < 0) {
        //not valid key
        return -EINVAL;
    }

    if (key == IPC_PRIVATE) {

        tag_descriptor = create_tag(key, permissions);
        printk("tag descriptor %d\n", tag_descriptor);
        if (tag_descriptor < 0) {
            printk(KERN_INFO "%s : Unable to create a new tag.\n", MODNAME);
            //tag creation failed
            return -ENOMEM;
        }

        return tag_descriptor;
    }
    /* use xor funtions a xor (b xor a ) = a to isolate a command bit */
    if ((command ^ IPC_EXCL) == IPC_CREAT || command == IPC_CREAT) {
        printk("CREAT CASE...");
        /*case of IPC_CREAT | IPC_EXCL  or just IPC_CREAT */
        if (down_write_killable(&key_list_sem) == -EINTR) return -EINTR;

        if (key_list[key] != -1) {
            //corresponding tag exists;
            tag_descriptor = key_list[key];
            up_write(&key_list_sem);

            /*case of IPC_CREAT | IPC_EXCL */
            if ((command ^ IPC_CREAT) == IPC_EXCL) {
                printk("eexcl case\n");
                //return error because was specified IPC_EXCL
                return -EEXIST;
            }
            printk("tag descriptor %d\n", tag_descriptor);
            return tag_descriptor;

        }

        tag_descriptor = create_tag(key, permissions);
        printk("tag descriptor %d\n", tag_descriptor);
        if (tag_descriptor < 0) {
            printk(KERN_INFO "%s : Unable to create a new tag.", MODNAME);
            //tag creation failed
            return -ENOMEM;
        }

        //modify key-list: insert a new tag associated to the key
        key_list[key] = tag_descriptor;
        //release lock
        up_write(&key_list_sem);

        return tag_descriptor;

    }

    /*not valid command was specified */

    return -EINVAL;

}

int tag_send(int tag, int level, char *buffer, size_t size) {
    tag_ptr_t my_tag;
    char *msg;
    int grace_epoch, next_epoch;
    unsigned long res;

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

            if (down_write_killable(&(my_tag->msg_rcu_util_list[level]->sem)) == -EINTR) {
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);
                return -EINTR;
            }

            if (size == 0) {
                /* nothing to copy*/
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->sem));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);

                /* zero lenght messages are anyhow allowed*/
                return 0;
            }

            /*  alloc memory to copy the content */
            msg = (char *) kzalloc(size, GFP_KERNEL);
            if (msg == NULL) {
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->sem));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);
                /* unable to allocate memory*/
                return -ENOMEM;
            }


            //start to copy the message
            res = copy_from_user(msg, buffer, size);
            asm volatile ("mfence":: : "memory");
            if (res != 0) {
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->sem));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);

                kfree(msg);
                return -EFAULT;
            }

            my_tag->msg_store[level]->msg = msg;
            my_tag->msg_store[level]->size = size;
            asm volatile ("sfence":: : "memory");


            grace_epoch = next_epoch = my_tag->msg_rcu_util_list[level]->current_epoch;
            my_tag->msg_rcu_util_list[level]->awake[grace_epoch] = YES;

            // now change epoch still under write lock
            next_epoch += 1;
            next_epoch = next_epoch % 2;
            my_tag->msg_rcu_util_list[level]->current_epoch = next_epoch;
            my_tag->msg_rcu_util_list[level]->awake[next_epoch] = NO;
            asm volatile ("mfence":: : "memory");

            /* wake up all thread waiting on the queue corresponding to the grace_epoch */
            wake_up_all(&my_tag->the_queue_head[level][grace_epoch]);

            while (my_tag->msg_rcu_util_list[level]->standings[grace_epoch] > 0) schedule();

            /* here all readerers on the grace_epoch consumed the message */
            /* restore default values */
            my_tag->msg_store[level]->msg = NULL;
            my_tag->msg_store[level]->size = 0;

            /* release write lock on the message buffer of the corresponding level */
            up_write(&(my_tag->msg_rcu_util_list[level]->sem));
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);

            kfree(msg);

            return 0;


        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);
            /* denied permission */
            return -EACCES;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag].tag_node_rwsem);
        /* tag specified not exists */
        return -ENOENT;
    }


}


int tag_receive(int tag, int level, char *buffer, size_t size) {
    int my_epoch_msg, my_epoch_awake, event_wq_ret;
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

            my_epoch_msg = my_tag->msg_rcu_util_list[level]->current_epoch;
            my_epoch_awake = my_tag->awake_rcu_util->current_epoch;

            __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], 1);
            __sync_fetch_and_add(&my_tag->awake_rcu_util->standings[my_epoch_awake], 1);

            /* wait event queues are used to selectively awake threads on some conditions*/
            event_wq_ret = wait_event_interruptible(my_tag->the_queue_head[level][my_epoch_msg],

                                                    my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] ==
                                                    YES /* case of message arriving */

                                                    ||

                                                    my_tag->awake_rcu_util->awake[my_epoch_awake] ==
                                                    YES /* case of awake all */
            );

            /*operation can fail also because of the delivery of a Posix signal*/
            if (event_wq_ret == -ERESTARTSYS) {
                //we have been awoken by a signal
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                __sync_fetch_and_add(&my_tag->awake_rcu_util->standings[my_epoch_awake], -1);

                up_read(&tag_list[tag].tag_node_rwsem);
                return -EINTR;

            } else if (my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] == YES) {
                /* let's read the incoming message */
                __sync_fetch_and_add(&my_tag->awake_rcu_util->standings[my_epoch_awake], -1);

                if (my_tag->msg_store[level]->size > size) {
                    // provided buffer is not large enough to copy the content of the message
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                    up_read(&tag_list[tag].tag_node_rwsem);

                    return -ENOBUFS;
                }

                ret = copy_to_user(buffer, my_tag->msg_store[level]->msg, my_tag->msg_store[level]->size);
                asm volatile ("mfence":: : "memory");
                if (ret != 0) {
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                    up_read(&tag_list[tag].tag_node_rwsem);

                    return -EFAULT;
                }

                ret = my_tag->msg_store[level]->size;

                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);

                up_read(&tag_list[tag].tag_node_rwsem);

                return (int) ret;

            } else if (my_tag->awake_rcu_util->awake[my_epoch_awake] == YES) {
                /* we have been awoken by AWAKEALL routine */
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                __sync_fetch_and_add(&my_tag->awake_rcu_util->standings[my_epoch_awake], -1);

                up_read(&tag_list[tag].tag_node_rwsem);
                /*this is not an error condition but 0 bytes has been copied because of awake all event*/
                return 0;

            }


        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);
            /* denied permission */
            return -EACCES;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag].tag_node_rwsem);
        /* tag specified not exists */
        return -ENOENT;
    }

    /*redundant ... */
    up_read(&tag_list[tag].tag_node_rwsem);
    return -EFAULT;

}

int tag_ctl(int tag, int command) {
    int grace_epoch, next_epoch, level, ret_key;
    tag_ptr_t my_tag;
    if (tag < 0 || tag >= max_tg) {
        /* Invalid Arguments error */
        return -EINVAL;
    }


    if (command == AWAKE_ALL) {
        /* take read lock to avoid that someone deletes the tag entry during my job*/
        if (down_read_killable(&tag_list[tag].tag_node_rwsem) == -EINTR) return -EINTR;

        my_tag = tag_list[tag].tag_ptr;
        if (my_tag != NULL) {

            if (GOT_PERMISSION(my_tag->uid.val, my_tag->perm)) {

                /*write lock to protect against concurrent threads executing awake all */
                if (down_write_killable(&(my_tag->awake_rcu_util->sem)) == -EINTR) {
                    /*release read lock on the tag_list i-th entry previously obtained*/
                    up_read(&tag_list[tag].tag_node_rwsem);
                    return -EINTR;
                }

                grace_epoch = next_epoch = my_tag->awake_rcu_util->current_epoch;
                my_tag->awake_rcu_util->awake[grace_epoch] = YES;

                // now change epoch still under write lock
                next_epoch += 1;
                next_epoch = next_epoch % 2;
                my_tag->awake_rcu_util->current_epoch = next_epoch;
                /* all threads that are going to arrive belong to the new epoch and they won't be awoken*/
                my_tag->awake_rcu_util->awake[next_epoch] = NO;
                asm volatile ("mfence":: : "memory");

                for (level = 0; level < LEVELS; level++) {
                    /* wake up all thread waiting on the queue; independently of the level*/
                    wake_up_all(&my_tag->the_queue_head[level][grace_epoch]);
                    wake_up_all(&my_tag->the_queue_head[level][next_epoch]);
                }


                while (my_tag->awake_rcu_util->standings[grace_epoch] > 0) schedule();

                /* release lock previously aquired */
                up_write(&my_tag->awake_rcu_util->sem);
                up_read(&tag_list[tag].tag_node_rwsem);

                return 0;

            } else {
                /*permission denided case*/
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag].tag_node_rwsem);
                return -EACCES;
            }
        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag].tag_node_rwsem);
            /* tag specified not exists */
            return -ENOENT;
        }


    }
    /* use xor funtions a xor (b xor a ) = a to isolate a command bit */
    if ((command ^ IPC_NOWAIT) == IPC_RMID || command == IPC_RMID) {
        /*case of REMOVE | IPC_NOWAIT  or just REMOVE */
        if ((command ^ IPC_RMID) == IPC_NOWAIT) {
            /* case of REMOVE | IPC_NOWAIT*/

            // every reader and every sender currently working with this tag takes a read lock
            // we obtain the write lock when neither readers and writers are  here anymore
            if (down_write_trylock(&tag_list[tag].tag_node_rwsem)) {
                // let's remove the tag
                ret_key = remove_tag(tag, 1);
                up_write(&tag_list[tag].tag_node_rwsem);
                return ret_key;

            } else {
                /*tag cannot be removed because it is busy and IPC_NOWAIT is specified */
                return -EBUSY;
            }
        }


        // every reader and every sender currently working with this tag takes a read lock
        // we obtain the write lock when neither readers and writers are  here anymore
        if (down_write_killable(&tag_list[tag].tag_node_rwsem) == -EINTR) return -EINTR;

        ret_key = remove_tag(tag, 0);

        up_write(&tag_list[tag].tag_node_rwsem);

        return ret_key;

    }



    // if we arrive here an invalid command was specified
    return -EINVAL;

}


void init_rcu_util(rcu_util_ptr rcu_util) {
    init_rwsem(&rcu_util->sem);
    rcu_util->standings[0] = 0;
    rcu_util->standings[1] = 0;
    rcu_util->awake[0] = NO;
    rcu_util->awake[1] = NO;
    rcu_util->current_epoch = 0;
}

int create_tag(int in_key, int permissions) {
    rcu_util_ptr new_awake_rcu, new_msg_rcu;
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


                //rcu util initialization
                new_awake_rcu = kzalloc(sizeof(struct rcu_util), GFP_KERNEL);
                if (new_awake_rcu == NULL) {
                    tag_list[i].tag_ptr = NULL;
                    up_write(&tag_list[i].tag_node_rwsem);
                    kfree(new_tag);
                    return -ENOMEM;
                }
                init_rcu_util(new_awake_rcu);
                new_tag->awake_rcu_util = new_awake_rcu;


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
    return -ENOKEY;

}


void tag_cleanup_mem(tag_ptr_t tag) {
    int i;
    if (tag == NULL) return;

    if (tag->awake_rcu_util != NULL) {
        kfree(tag->awake_rcu_util);
    }

    for (i = 0; i < LEVELS; i++) {
        if (tag->msg_store[i] != NULL) kfree(tag->msg_store[i]);
        if (tag->msg_rcu_util_list[i] != NULL) kfree(tag->msg_rcu_util_list[i]);
    }

    kfree(tag);
}

/**

 * take write lock on tag_list[tag]->tag_node_rwsem OUTSIDE of this function and use nowait=1 to provide a nonblocking
 * behavior; if nowait is specified and the resource is not immediately available the operation abort and -EBUSY returned
 */
int remove_tag(int tag, int nowait) {
    int ret_key;
    tag_ptr_t my_tag = tag_list[tag].tag_ptr;
    if (my_tag != NULL) {
        ret_key = my_tag->key;

        if (GOT_PERMISSION(my_tag->uid.val, my_tag->perm)) {

            if (ret_key != IPC_PRIVATE) {
                if (nowait) {
                    if (down_write_trylock(&key_list_sem)) {
                        key_list[ret_key] = -1;
                        up_write(&key_list_sem);
                    } else {
                        return -EBUSY;
                    }

                } else {
                    if (down_write_killable(&key_list_sem) == -EINTR) return -EINTR;
                    key_list[ret_key] = -1;
                    up_write(&key_list_sem);

                }
            }
            /* delete the tag from the tag_list */
            tag_list[tag].tag_ptr = NULL;
            asm volatile ("mfence");
            /*cleanup memory previously allocated*/
            tag_cleanup_mem(my_tag);

            return ret_key;

        } else {
            //permission denided case
            return -EACCES;
        }
    } else {
        /* this tag is not present */
        return -ENOENT;
    }
}

