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

#include "tag_flags.h"
#include <sys/ipc.h>
#include <linux/slab.h>

extern tag_node_ptr *tag_list;
extern int *key_list;
extern struct rw_semaphore key_list_sem;
extern int max_key;
extern int max_tags;


int create_tag(int in_key, int permissions);


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
        if (down_write_killable(&key_list_sem)) return -EINTR;

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
    int grace_epoch, temp;
    unsigned long res;

    if (tag < 0 || tag > max_tags || level >= LEVELS || level < 0 || buffer == NULL || size < 0) {
        /* Invalid Arguments error */
        return -EINVAL;
    }
    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag]->tag_node_rwsem)) return -EINTR;

    tag_ptr_t my_tag = tag_list[tag]->ptr;
    if (my_tag != NULL) {
        /* pemisson check */
        if (
            /* only creator user can access to this tag and the current user correspond to him */
                (my_tag->perm && my_tag->uid.val == current_uid().val) ||
                /* all user can access to this tag */
                !my_tag->perm
                ) {

            if (down_write_killable(&(my_tag->msg_rcu_util_list[level]->_lock))) {
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);
                return -EINTR;
            }

            if (size == 0) {
                /* nothing to copy*/
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->_lock));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);

                /* zero lenght messages are anyhow allowed*/
                return 0;
            }


            //start to copy the message

            msg = (char *) kzalloc(size, GFP_KERNEL);
            if (msg == NULL) {
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->_lock));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);
                /* unable to allocate memory*/
                return -ENOMEM;
            }


            res = copy_from_user(msg, buffer, size);
            asm volatile ("mfence":: : "memory");
            if (res != 0) {
                /* release write lock on the message buffer of the corresponding level */
                up_write(&(my_tag->msg_rcu_util_list[level]->_lock));
                /*release r_lock on the tag_list i-th entry previously obtained*/
                up_read(&tag_list[tag]->tag_node_rwsem);

                kfree(msg);
                return -EFAULT;
            }

            my_tag->msg_store[level]->msg = msg;
            my_tag->msg_store[level]->size = size;
            asm volatile ("sfence":: : "memory");

            // now change epoch still under write lock
            grace_epoch = temp = my_tag->msg_rcu_util_list[level]->current_epoch;
            temp += 1;
            temp = temp % 2;
            my_tag->msg_rcu_util_list[level]->current_epoch = temp;

            asm volatile ("mfence":: : "memory");

            /* wake up all thread waiting on the queue corresponding to the grace_epoch */
            wake_up_all(&my_tag->sync_conditional[level][grace_epoch]);

            while (my_tag->msg_rcu_util_list[level]->presence_counter[grace_epoch] > 0) schedule();

            /* here all readerers on the grace_epoch consumed the message */
            my_tag->msg_store[level]->msg = NULL;
            my_tag->msg_store[level]->size = 0;

            /* release write lock on the message buffer of the corresponding level */
            up_write(&(my_tag->msg_rcu_util_list[level]->_lock));
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
    int my_epoch_msg, my_epoch_synk, wait_res;
    unsigned long res;

    if (tag < 0 || tag > max_tags || level >= LEVELS || level < 0 || buffer == NULL || size < 0) {
        /* Invalid Arguments error */
        return -EINVAL;
    }

    /* take read lock to avoid that someone deletes the tag entry during my job*/
    if (down_read_killable(&tag_list[tag]->tag_node_rwsem)) return -EINTR;
    tag_ptr_t my_tag = tag_list[tag]->ptr;
    if (my_tag != NULL) {
        /* pemisson check */
        if (
            /* only creator user can access to this tag and the current user correspond to him */
                (my_tag->perm && my_tag->uid.val == current_uid().val) ||
                /* all user can access to this tag */
                !my_tag->perm
                ) {

            my_epoch_msg = my_tag->msg_rcu_util_list[level]->current_epoch;
            my_epoch_synk = my_tag->awake_rcu_util_list[level]->current_epoch;

            __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->presence_counter[my_epoch_msg], 1);
            __sync_fetch_and_add(&my_tag->awake_rcu_util_list[level]->presence_counter[my_epoch_synk], 1);


            wait_res = wait_event_interruptible(my_tag->sync_conditional[level][my_epoch_msg],
                                                (my_tag->msg_rcu_util_list[level]->sync[my_epoch_msg] == 0x1)
                                                ||
                                                ( my_tag->awake_rcu_util_list[level]->sync[my_epoch_synk] == 0x1));

            if (wait_res == -ERESTARTSYS){
                //we have been awoken by a signal
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->presence_counter[my_epoch_msg], -1);
                __sync_fetch_and_add(&my_tag->awake_rcu_util_list[level]->presence_counter[my_epoch_synk], -1);

                up_read(&tag_list[tag]->tag_node_rwsem);
                return -EINTR;

            }else if(my_tag->msg_rcu_util_list[level]->sync[my_epoch_synk] == 0x1){
                /* we have been awoken by AWAKEALL routine */
                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->presence_counter[my_epoch_msg], -1);
                __sync_fetch_and_add(&my_tag->awake_rcu_util_list[level]->presence_counter[my_epoch_synk], -1);

                up_read(&tag_list[tag]->tag_node_rwsem);
                /*this is not an error condition but 0 bytes has been copied */
                return 0;

            }else if (my_tag->awake_rcu_util_list[level]->sync[my_epoch_synk] == 0x1){
                /* let's read the incoming message */
                __sync_fetch_and_add(&my_tag->awake_rcu_util_list[level]->presence_counter[my_epoch_synk], -1);

                if (my_tag->msg_store[level]->size > size){
                    // provided buffer is not large enough to copy the content of the message
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->presence_counter[my_epoch_msg], -1);
                    up_read(&tag_list[tag]->tag_node_rwsem);

                    return -ENOBUFS;
                }

                res = copy_to_user(buffer, my_tag->msg_store[level]->msg, my_tag->msg_store[level]->size);
                asm volatile ("mfence":: : "memory");
                if (res != 0){
                    __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->presence_counter[my_epoch_msg], -1);
                    up_read(&tag_list[tag]->tag_node_rwsem);

                    return -EFAULT;
                }

                res = my_tag->msg_store[level]->size;

                __sync_fetch_and_add(&my_tag->msg_rcu_util_list[level]->presence_counter[my_epoch_msg], -1);
                up_read(&tag_list[tag]->tag_node_rwsem);

                return (int) res;

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

void init_rcu_util(rcu_util_ptr rcu_util) {
    rcu_util->presence_counter[0] = 0; \
    rcu_util->presence_counter[1] = 0; \
    rcu_util->sync[0] = 0x0; \
    rcu_util->sync[1] = 0x1;\
    rcu_util->current_epoch = 0x0;
    init_rwsem(&rcu_util->_lock);
}

int create_tag(int in_key, int permissions) {

    int i = 0;
    tag_ptr_t new_tag;
    for (; i < max_tags; i++) {
        if (down_write_trylock(&tag_list[i]->tag_node_rwsem)) {
            //succesfull , lock aquired

            if (tag_list[i]->ptr == NULL) {
                new_tag = kzalloc(sizeof(struct tag_t), GFP_KERNEL);
                if (new_tag == NULL) {
                    //unable to allocate, release lock and return error
                    up_write(&tag_list[i]->tag_node_rwsem);
                    return -ENOMEM;
                }


                new_tag->key = in_key;
                new_tag->uid.val = current_uid().val;
                if (permissions > 0) new_tag->perm = 0x1;
                else new_tag->perm = 0x0;

                int j;
                for (j = 0; j < LEVELS; j++) {
                    new_tag->msg_store[j]->msg = NULL;
                    new_tag->msg_store[j]->size = 0;
                    init_rcu_util(new_tag->msg_rcu_util_list[j]);
                    init_rcu_util(new_tag->awake_rcu_util_list[j]);
                    init_waitqueue_head(&new_tag->sync_conditional[j][0]);
                    init_waitqueue_head(&new_tag->sync_conditional[j][1]);
                }

                tag_list[i]->ptr = new_tag;
                asm volatile ("sfence":: : "memory");

                up_write(&tag_list[i]->tag_node_rwsem);

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