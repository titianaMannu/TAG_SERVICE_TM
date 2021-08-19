/**
 * @file tag_service.c
 * @brief todo insert a detailed description  description
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
#include <stdbool.h>
#include "tag_flags.h"
#include <sys/ipc.h>

extern tag_node_ptr *tag_list;
extern int *key_list;
extern struct rw_semaphore key_list_sem;
extern int max_key;
extern int max_tg;


int create_tag(int in_key, int permissions);

int remove_tag(int tag);

int tag_get(int key, int command, int permissions) {
    int tag_descriptor;
    if (key != IPC_PRIVATE && (key > max_key || key < 0)) {
        //not valid key
        return -ENOKEY;
    }

    if (key == IPC_PRIVATE) {

        tag_descriptor = create_tag(key, permissions);

        if (tag_descriptor < 0) {
            printk(KERN_INFO "%s : Unable to create a new tag.", MODNAME);
            //tag creation failed
            return -ENOMEM;
        }

        return tag_descriptor;
    }
        /* use xor funtions a xor (b xor a ) = a to isolate a command bit */
    else if ((command ^ IPC_EXCL) == IPC_CREAT || command == IPC_CREAT) {
        /*case of IPC_CREAT | IPC_EXCL  or just IPC_CREAT */
        if (down_write_killable(&key_list_sem) == -EINTR) return -EINTR;

        if (key_list[key] != -1) {
            //corresponding tag exists;
            tag_descriptor = key_list[key];
            up_write(&key_list_sem);

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
            //tag creation failed
            return -ENOMEM;
        }

        //modify key-list: insert a new tag associated to the key
        key_list[key] = tag_descriptor;
        //release lock
        up_write(&key_list_sem);

        return tag_descriptor;

    }


}

int tag_send(int tag, int level, char *buffer, size_t size) {

    char *msg;
    int grace_epoch, next_epoch;
    unsigned long res;

    if (tag < 0 || tag >= max_tg || level >= LEVELS || level < 0 || buffer == NULL || size < 0) {
        /* Invalid Arguments error */
        return -EINVAL;
    }
    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag]->tag_node_rwsem) == -EINTR) return -EINTR;

    tag_ptr_t my_tag = tag_list[tag]->tag_ptr;
    if (my_tag != NULL) {
        /* permisson check */
        if (
            /* only creator user can access to this tag and the current user correspond to him */
                (my_tag->perm && my_tag->uid.val == current_uid().val) ||
                /* all user can access to this tag */
                !my_tag->perm
                ) {

            if (down_write_killable(&(my_tag->msg_rcu_util_list[level]->sem)) == -EINTR) {
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);
                return -EINTR;
            }

            if (size == 0) {
                /* nothing to copy*/
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->sem));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);

                /* zero lenght messages are anyhow allowed*/
                return 0;
            }


            //start to copy the message
            msg = (char *) kzalloc(size, GFP_KERNEL);
            if (msg == NULL) {
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->sem));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);
                /* unable to allocate memory*/
                return -ENOMEM;
            }


            res = copy_from_user(msg, buffer, size);
            asm volatile ("mfence":: : "memory");
            if (res != 0) {
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->sem));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);

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
            wake_up_all(&my_tag->sync_conditional[level][grace_epoch]);

            while (my_tag->msg_rcu_util_list[level]->standings[grace_epoch] > 0) schedule();

            /* here all readerers on the grace_epoch consumed the message */
            /* restore default values */
            my_tag->msg_store[level]->msg = NULL;
            my_tag->msg_store[level]->size = 0;

            /* release write lock on the message buffer of the corresponding level */
            up_write(&(my_tag->msg_rcu_util_list[level]->sem));
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag]->tag_node_rwsem);

            kfree(msg);

            return 0;


        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag]->tag_node_rwsem);
            /* denied permission */
            return -EACCES;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag]->tag_node_rwsem);
        /* tag specified not exists */
        return -EFAULT;
    }


}


int tag_receive(int tag, int level, char *buffer, size_t size) {
    int my_epoch_msg, my_epoch_awake, event_wq_ret;
    unsigned long ret;

    if (tag < 0 || tag >= max_tg || level >= LEVELS || level < 0 || buffer == NULL || size < 0) {
        /* Invalid Arguments error */
        return -EINVAL;
    }

    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag]->tag_node_rwsem) == -EINTR) return -EINTR;

    tag_ptr_t my_tag = tag_list[tag]->tag_ptr;
    if (my_tag != NULL) {
        /* permisson check */
        if (
            /* only creator user can access to this tag and the current user correspond to him */
                (my_tag->perm && my_tag->uid.val == current_uid().val) ||
                /* all user can access to this tag */
                !my_tag->perm
                ) {

            my_epoch_msg = my_tag->msg_rcu_util_list[level]->current_epoch;
            my_epoch_awake = my_tag->awake_rcu_util_list->current_epoch;

            __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], 1);
            __sync_fetch_and_add(&my_tag->awake_rcu_util_list->standings[my_epoch_awake], 1);

            /* wait event queues are used to selectively awake threads on some conditions*/
            event_wq_ret = wait_event_interruptible(my_tag->sync_conditional[level][my_epoch_msg],

                                                    my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] ==
                                                    YES /* case of message arriving */

                                                    ||

                                                    my_tag->awake_rcu_util_list->awake[my_epoch_awake] ==
                                                    YES /* case of awake all */
            );

            if (event_wq_ret == -ERESTARTSYS) {
                //we have been awoken by a signal
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                __sync_fetch_and_add(&my_tag->awake_rcu_util_list->standings[my_epoch_awake], -1);

                up_read(&tag_list[tag]->tag_node_rwsem);
                return -EINTR;

            } else if (my_tag->msg_rcu_util_list[level]->awake[my_epoch_msg] == YES) {
                /* let's read the incoming message */
                __sync_fetch_and_add(&my_tag->awake_rcu_util_list->standings[my_epoch_awake], -1);

                if (my_tag->msg_store[level]->size > size) {
                    // provided buffer is not large enough to copy the content of the message
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                    up_read(&tag_list[tag]->tag_node_rwsem);

                    return -ENOBUFS;
                }

                ret = copy_to_user(buffer, my_tag->msg_store[level]->msg, my_tag->msg_store[level]->size);
                asm volatile ("mfence":: : "memory");
                if (ret != 0) {
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                    up_read(&tag_list[tag]->tag_node_rwsem);

                    return -EFAULT;
                }

                ret = my_tag->msg_store[level]->size;

                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);

                up_read(&tag_list[tag]->tag_node_rwsem);

                return (int) ret;

            } else if (my_tag->awake_rcu_util_list->awake[my_epoch_awake] == YES) {
                /* we have been awoken by AWAKEALL routine */
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->standings[my_epoch_msg], -1);
                __sync_fetch_and_add(&my_tag->awake_rcu_util_list->standings[my_epoch_awake], -1);

                up_read(&tag_list[tag]->tag_node_rwsem);
                /*this is not an error condition but 0 bytes has been copied because of awake all event*/
                return 0;

            }


        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag]->tag_node_rwsem);
            /* denied permission */
            return -EACCES;
        }
    } else {
        /*release r_lock on the tag_list i-th entry previously obtained*/
        up_read(&tag_list[tag]->tag_node_rwsem);
        /* tag specified not exists */
        return -EFAULT;
    }


}

int tag_ctl(int tag, int command) {
    int grace_epoch, next_epoch, level;
    if (tag < 0 || tag >= max_tg) {
        /* Invalid Arguments error */
        return -EINVAL;
    }


    if (command == AWAKE_ALL) {
        /* take read lock to avoid that someone deletes the tag entry during my job*/
        if (down_read_killable(&tag_list[tag]->tag_node_rwsem) == -EINTR) return -EINTR;

        tag_ptr_t my_tag = tag_list[tag]->tag_ptr;
        if (my_tag != NULL) {

            if (
                /* only creator user can access to this tag and the current user correspond to him */
                    (my_tag->perm && my_tag->uid.val == current_uid().val) ||
                    /* all user can access to this tag */
                    !my_tag->perm) {

                if (down_write_killable(&(my_tag->awake_rcu_util_list->sem)) == -EINTR) {
                    /*release read lock on the tag_list i-th entry previously obtained*/
                    up_read(&tag_list[tag]->tag_node_rwsem);
                    return -EINTR;
                }

                grace_epoch = next_epoch = my_tag->awake_rcu_util_list->current_epoch;
                my_tag->awake_rcu_util_list->awake[grace_epoch] = YES;

                // now change epoch still under write lock
                next_epoch += 1;
                next_epoch = next_epoch % 2;
                my_tag->awake_rcu_util_list->current_epoch = next_epoch;
                /* all threads that are going to arrive belong to the new epoch and they wont be awoken*/
                my_tag->awake_rcu_util_list->awake[next_epoch] = NO;
                asm volatile ("mfence":: : "memory");

                for (level = 0; level < LEVELS; level++) {
                    /* wake up all thread waiting on the queue; independently of the level*/
                    wake_up_all(&my_tag->sync_conditional[level][grace_epoch]);
                    wake_up_all(&my_tag->sync_conditional[level][next_epoch]);
                }


                while (my_tag->awake_rcu_util_list->standings[grace_epoch] > 0) schedule();

                /* release lock previously aquired */
                up_write(&my_tag->awake_rcu_util_list->sem);
                up_read(&tag_list[tag]->tag_node_rwsem);

                return 0;

            } else {
                /*permission denided case*/
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);
                return -EACCES;
            }
        } else {
            /*release r_lock on the tag_list i-th entry previously obtained*/
            up_read(&tag_list[tag]->tag_node_rwsem);
            /* tag specified not exists */
            return -EFAULT;
        }


    }

    if (command == REMOVE) {
        return remove_tag(tag);
    }

    // if we arrive here an invalid command was specified
    return -EINVAL;

}


void init_rcu_util(rcu_util_ptr rcu_util) {
    rcu_util->standings[0] = 0; \
    rcu_util->standings[1] = 0; \
    rcu_util->awake[0] = 0x0; \
    rcu_util->awake[1] = 0x1;\
    rcu_util->current_epoch = 0x0;
    init_rwsem(&rcu_util->sem);
}

int create_tag(int in_key, int permissions) {

    int i = 0;
    tag_ptr_t new_tag;
    for (; i < max_tg; i++) {
        if (down_write_trylock(&tag_list[i]->tag_node_rwsem)) {
            //succesfull , lock acquired

            if (tag_list[i]->tag_ptr == NULL) {
                new_tag = kzalloc(sizeof(struct tag_t), GFP_KERNEL);
                if (new_tag == NULL) {
                    //unable to allocate, release lock and return error
                    up_write(&tag_list[i]->tag_node_rwsem);
                    return -ENOMEM;
                }


                new_tag->key = in_key;
                new_tag->uid.val = current_uid().val;
                if (permissions > 0) new_tag->perm = true;
                else new_tag->perm = false;

                int j;
                for (j = 0; j < LEVELS; j++) {
                    //message buffer initialization
                    new_tag->msg_store[j]->msg = NULL;
                    new_tag->msg_store[j]->size = 0;
                    //rcu util initialization
                    init_rcu_util(new_tag->msg_rcu_util_list[j]);
                    //wait event queues initialization
                    init_waitqueue_head(&new_tag->sync_conditional[j][0]);
                    init_waitqueue_head(&new_tag->sync_conditional[j][1]);
                }
                //rcu util initialization
                init_rcu_util(new_tag->awake_rcu_util_list);

                tag_list[i]->tag_ptr = new_tag;
                asm volatile ("sfence":: : "memory");

                up_write(&tag_list[i]->tag_node_rwsem);
                // return a tag descriptor
                return i;


            } else {

                up_write(&tag_list[i]->tag_node_rwsem);
                continue;

            }

        }

    }

    //research failed ... there aren't free tags to use
    //if we can't get write lock on a tag it means that someone else is doing something with that tag
    //so it isn't free and you cannot insert a new one.
    return -ENOMEM;

}

int remove_tag(int tag) {
    /*
     * every reader and every sender currently working with this tag takes a read lock
     * we obtain the write lock when neither readers and writers are  here anymore
     */
    if (down_write_killable(&tag_list[tag]->tag_node_rwsem) == -EINTR) return -EINTR;


    tag_ptr_t my_tag = tag_list[tag]->tag_ptr;
    if (my_tag != NULL) {

        if (
            /* only creator user can access to this tag and the current user correspond to him */
                (my_tag->perm && my_tag->uid.val == current_uid().val) ||
                /* all user can access to this tag */
                !my_tag->perm
                ) {


            if (my_tag->key != IPC_PRIVATE) {

                if (down_write_killable(&key_list_sem) == -EINTR) return -EINTR;

                /* delete the tag from the key_list */
                /* linear research  */
                int i = 0;
                for (; i < max_key; i++) {
                    if (key_list[i] == tag) {
                        key_list[i] = -1;
                        break;
                    }
                }
                up_write(&key_list_sem);
            }

            /* delete the tag from the tag_list */
            tag_list[tag] = NULL;
            asm volatile ("mfence");
            kfree(my_tag);

            up_write(&tag_list[tag]->tag_node_rwsem);
            return 0;

        } else {
            //permission denided case
            up_write(&tag_list[tag]->tag_node_rwsem);
            return -EACCES;
        }
    } else {
        up_write(&tag_list[tag]->tag_node_rwsem);
        /* this tag is not present */
        return -EIDRM;
    }
}