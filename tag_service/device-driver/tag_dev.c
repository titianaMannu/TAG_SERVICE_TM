//
// Created by tiziana on 9/1/21.
//

#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/rwsem.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include "tag_dev.h"

extern tag_node_ptr tag_list;
extern int major_number;
extern unsigned int max_tg;
static DEFINE_MUTEX(device_state);
tag_status_ptr_t status_list;
dev_object_t info;

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = open_tag_status,
        .read = read_tag_status,
        .write = write_tag_status,
        .release = release_tag_status
};

/**
 * @description This function opens a new session for the device file and build the information needed.
 * Be carefull, this device driver cannot be accessed in time sharing (single istance)
 * if someone else try to open a new session without closing the current one this will cause an error condition.
 *
 * @param inode file inode
 * @param file file struct
 * @return 0 or errno is set to a correct value
 */
int open_tag_status(struct inode *inode, struct file *file) {
    int i, j, res;
    unsigned int minor;
    tag_ptr_t my_tag;
    if (inode == NULL || file == NULL) {
        /*invalid argument*/
        return -EINVAL;
    }
    minor = file->f_inode->i_rdev;
    if (!mutex_trylock(&device_state)) {
        /*this device file is single instance*/
        return -EBUSY;
    }

    status_list = kzalloc(sizeof(tag_status_t) * max_tg, GFP_KERNEL);
    if (status_list == NULL) {
        printk(KERN_INFO "Tag-Driver unable to allocate memory\n");
        return -ENOMEM;
    }

    /*let's retrieve status informations*/
    for (i = 0; i < max_tg; i++) {
        /* take read lock to avoid that someone deletes the tag entry during my job*/
        if (!down_read_trylock(&tag_list[i].tag_node_rwsem)) {
            //contention! this means that a remover or a creator is here, the go on
            status_list[i].present = false;
            continue;
        }
        // this is a snapshot and I don't care about concurrency
        my_tag = tag_list[i].tag_ptr;
        if (my_tag != NULL) {
            status_list[i].present = true;
            status_list[i].key = my_tag->key;
            status_list[i].uid_owner = my_tag->uid;
            for (j = 0; j < LEVELS; j++) {
                //consider both current_epoch and next_epoch
                status_list[i].standing_readers[j] =
                        my_tag->msg_rcu_util_list[j]->standings[0] + my_tag->msg_rcu_util_list[j]->standings[1];
            }
        } else {
            status_list[i].present = false;
        }

        up_read(&tag_list[i].tag_node_rwsem);
    }
    /*Here we have collected all the infrmation needed to build the text*/
    res = build_content();
    if (res < 0) {
        kfree(status_list);
        mutex_unlock(&device_state);
        return res;
    }

    printk("%s : %s succesfully opened with major=%d and minor=%d\n", MODNAME, DEVICE_NAME, major_number, minor);
    return 0;
}

/**
 * @description Builds a text with the information collected from the tag_list of the tag_service.
 * @return integer corresponding to the number of bytes that have been written
 */
int build_content(void) {
    int i, j, written;
    char *temp_text, *text;
    /*alloc a potentially big amount of memory*/
    text = vzalloc(max_tg * LEVELS * LINE_LEN);
    if (text == NULL) {
        printk(KERN_INFO "%s : unable to allocate memory\n", DEVICE_NAME);
        return -ENOMEM;
    }

    temp_text = text;
    written = 0;
    for (i = 0; i < max_tg; i++) {
        if (status_list[i].present) {
            for (j = 0; j < LEVELS; j++) {
                /*consider only the levels for wich there are standing readers*/
                if (status_list[i].standing_readers[j] != 0) {
                    written += sprintf(temp_text, "key=%d\towner=%d\tlevel=%d\treaders=%ld\n",
                                       status_list[i].key,
                                       status_list[i].uid_owner.val,
                                       j,
                                       status_list[i].standing_readers[j]);
                    temp_text += written;
                }
            }
        }
    }

    info.content = text;
    info.content_size = written;
    return written;
}

/**
 * @description Allows reading informations about the tag-service collected during the open operation.
 * @param filp file struct
 * @param buff destination user space buffer
 * @param len buffer length
 * @param off reading starting point
 * @return number of bytes read
 */
ssize_t read_tag_status(struct file *filp, char *buff, size_t len, loff_t *off) {
    unsigned long res;
    if (info.content == NULL) {
        /*no available informations*/
        return -EBADF;
    }

    if (buff == NULL || off == NULL) {
        return -EINVAL;
    }

    if (*off > info.content_size)//EOF case
        return 0;

    if ((info.content_size - *off) < len) len = info.content_size - *off;
    /*copy the text into a user buffer*/
    res = copy_to_user(buff, &(info.content[*off]), len);

    *off += (ssize_t)(len - res);

    return (ssize_t)(len - res);
}

/**
 * @description release resources
 * @param inode dev file inode
 * @param file file struct
 * @return 0 on success
 */
int release_tag_status(struct inode *inode, struct file *file) {
    if (status_list != NULL) kfree(status_list);
    if (info.content != NULL) vfree(info.content);
    info.content_size = 0;
    mutex_unlock(&device_state);
    printk("%s : %s succesfully closed.\n", MODNAME, DEVICE_NAME);
    return 0;
}

/**
 * @description Not implemented yet
 */
ssize_t write_tag_status(struct file *filp, const char *buff, size_t len, loff_t *off) {
    /*Not implemented yet*/
    return -ENOSYS;
}