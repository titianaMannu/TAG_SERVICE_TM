/**
 * @file tag_main.c
 *
 * @description This file contains module initialization entry-point and cleanup for ther tag_service module.
 *
 * @author Tiziana Mannucci
 *
 * @mail titianamannucci@gmail.com
 *
 * @date 23/08/2021
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/cdev.h>
#include <linux/rwsem.h>
#include <linux/errno.h>
#include <linux/compiler.h>


#include "systbl_hack/systbl_hack.h"
#include "tag_flags.h"
#include "tag.h"
#include "device-driver/tag_dev.h"


/* Usage with Kernel >= 4.20 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#error "Required Kernel verison  >= 4.20."
#endif


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tiziana Mannucci <titianamannucci@gmail.com>");
MODULE_DESCRIPTION("This Module implements a message exchange service based on tags");

/*use this to hack the syscall table*/
struct module *systbl_hack_mod_ptr;

/* Driver major number. */
int major_number = 0;

module_param(major_number, int, S_IRUGO);
MODULE_PARM_DESC(major_number, "Major number for tag-service device driver.");

/* Max Keys*/
unsigned int max_key = MAX_KEY;

module_param(max_key, uint, S_IRUGO);
MODULE_PARM_DESC(max_key, "Total number of key provided.");

/* Max Tags */
unsigned int max_tg = MAX_TAG;

module_param(max_tg, uint, S_IRUGO);
MODULE_PARM_DESC(max_tg, "Total number of tags provided.");

/* Max message size. */
unsigned int msg_size = MSG_LEN;

module_param(msg_size, uint, S_IRUGO);
MODULE_PARM_DESC(msg_size, "Max message size.");

tag_node_ptr tag_list = NULL;
int *key_list = NULL;


int tag_get_nr; // tag_get syscall number
int tag_send_nr; //tag_send syscall number
int tag_receive_nr;// tag_receive syscall number
int tag_ctl_nr;// tag_ctl syscall number
extern struct file_operations fops;

__SYSCALL_DEFINEx(3, _tag_get, int, key, int, command, int, permissions) {
    int res;
    if (!try_module_get(THIS_MODULE)) return -ENOSYS;
    res = tag_get(key, command, permissions);
    module_put(THIS_MODULE);
    return res;
}

__SYSCALL_DEFINEx(4, _tag_send, int, tag, int, level, char *, buffer, size_t, size) {
    int res;
    if (!try_module_get(THIS_MODULE)) return -ENOSYS;
    res = tag_send(tag, level, buffer, size);
    module_put(THIS_MODULE);
    return res;
}

__SYSCALL_DEFINEx(4, _tag_receive, int, tag, int, level, char *, buffer, size_t, size) {
    int res;
    if (!try_module_get(THIS_MODULE)) return -ENOSYS;
    res = tag_receive(tag, level, buffer, size);
    module_put(THIS_MODULE);
    return res;

}

__SYSCALL_DEFINEx(2, _tag_ctl, int, tag, int, command) {
    int res;
    if (!try_module_get(THIS_MODULE)) return -ENOSYS;
    res = tag_ctl(tag, command);
    module_put(THIS_MODULE);
    return res;
}

/**
 * @description Initialize the module with all needed structures.
 * @return 0 or errno is set to the correct error code.
 */
int tag_service_init(void) {
    int i;
    printk(KERN_INFO "%s name = %s\n", MODNAME, THIS_MODULE->name);
    if (max_key > MAX_KEY) max_key = MAX_KEY;
    if (max_tg < MAX_TAG) max_tg = MAX_TAG;
    if (msg_size < MSG_LEN) msg_size = MSG_LEN;

    printk(KERN_INFO "%s: Initializing...\n", MODNAME);

    if (mutex_lock_interruptible(&module_mutex) == -EINTR) return -EINTR;

    systbl_hack_mod_ptr = find_module("systbl_hack");
    if (!systbl_hack_mod_ptr) {
        mutex_unlock(&module_mutex);
        return -ENOENT;
    }
    if (!try_module_get(systbl_hack_mod_ptr)) {
        mutex_unlock(&module_mutex);
        printk(KERN_INFO "%s : cannot find systbl_hack module\n", MODNAME);
        return -ENOENT;
    }
    mutex_unlock(&module_mutex);

    /*device driver registration*/
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_INFO "%s : error in driver registration\n", MODNAME);
        printk(KERN_INFO "%s : Failed initialization\n", MODNAME);
        module_put(systbl_hack_mod_ptr);
        return -1;
    }
    printk(KERN_INFO "%s : %s correctly mounted with major number %d\n", MODNAME, DEVICE_NAME, major_number);

    /* Global structs initialization */
    key_list = kzalloc(sizeof(int) * max_key, GFP_KERNEL);
    if (key_list == NULL) {
        printk(KERN_INFO "%s : Unable to allocate memory to create key list.\n", MODNAME);
        module_put(systbl_hack_mod_ptr);
        return -ENOMEM;
    }
    /*setup key_list like an empty list*/
    for (i = 0; i < max_key; i++) {
        key_list[i] = -1;
    }
    /*allocate memory for the tag list*/
    tag_list = (tag_node_ptr) kzalloc(sizeof(tag_node) * max_tg, GFP_KERNEL);
    if (tag_list == NULL) {
        printk(KERN_INFO "%s : Unable to allocate memory to create tags list.\n", MODNAME);
        kfree(key_list);
        module_put(systbl_hack_mod_ptr);
        return -ENOMEM;
    }

    for (i = 0; i < max_tg; i++) {
        init_rwsem(&tag_list[i].tag_node_rwsem);
        tag_list[i].tag_ptr = NULL;
    }


    /*insert the 4 system calls in the table */
    tag_get_nr = systbl_hack(__x64_sys_tag_get);
    if (tag_get_nr < 0) goto error_exit_point;

    tag_send_nr = systbl_hack(__x64_sys_tag_send);
    if (tag_send_nr < 0) goto error_exit_point;

    tag_receive_nr = systbl_hack(__x64_sys_tag_receive);
    if (tag_receive_nr < 0) goto error_exit_point;

    tag_ctl_nr = systbl_hack(__x64_sys_tag_ctl);
    if (tag_ctl_nr < 0) goto error_exit_point;

    printk(KERN_INFO "%s : tag_get at %d\n", MODNAME, tag_get_nr);
    printk(KERN_INFO "%s : tag_send at %d\n", MODNAME, tag_send_nr);
    printk(KERN_INFO "%s : tag_receive at %d\n", MODNAME, tag_receive_nr);
    printk(KERN_INFO "%s : tag_ctl at %d\n", MODNAME, tag_ctl_nr);

    printk(KERN_INFO "%s : module correctly mounted\n", MODNAME);
    return 0;

    error_exit_point:
    /*restore all the entry previously inserted; if not present anymore the systbl_entry_restore do nothing*/
    systbl_entry_restore(tag_get_nr, 1);
    systbl_entry_restore(tag_receive_nr, 1);
    systbl_entry_restore(tag_send_nr, 1);
    systbl_entry_restore(tag_ctl_nr, 1);
    printk(KERN_INFO "%s : Failed initialization\n", MODNAME);
    kfree(tag_list);
    kfree(key_list);
    if (major_number != 0) {
        unregister_chrdev(major_number, DEVICE_NAME);
    }
    module_put(systbl_hack_mod_ptr);
    return -1;
}

/**
 * @description Module Cleanup. Deletes from the system call table all the system calls previously inserted
 * and release all the resources allocated.
 */
void tag_service_clean(void) {
    int i;
    if (systbl_entry_restore(tag_get_nr, 1) == 0) {
        printk(KERN_INFO "%s : deleted tag_get at %d\n", MODNAME, tag_get_nr);
    }
    if (systbl_entry_restore(tag_receive_nr, 1) == 0) {
        printk(KERN_INFO "%s : deleted tag_send at %d\n", MODNAME, tag_send_nr);
    }
    if (systbl_entry_restore(tag_send_nr, 1) == 0) {
        printk(KERN_INFO "%s : deleted tag_receive at %d\n", MODNAME, tag_receive_nr);
    }
    if (systbl_entry_restore(tag_ctl_nr, 1) == 0) {
        printk(KERN_INFO "%s : deleted tag_ctl at %d\n", MODNAME, tag_ctl_nr);
    }

    if (major_number != 0) {
        printk(KERN_INFO "%s : unregister %s.\n", MODNAME, DEVICE_NAME);
        unregister_chrdev(major_number, DEVICE_NAME);
    }

    kfree(key_list);
    for (i = 0; i < max_tg; i++) {
        tag_cleanup_mem(tag_list[i].tag_ptr);
    }
    kfree(tag_list);
    module_put(systbl_hack_mod_ptr);
    printk(KERN_INFO "%s : clean system calls for tag-service ... exit.\n", MODNAME);
}

module_init(tag_service_init)
module_exit(tag_service_clean);