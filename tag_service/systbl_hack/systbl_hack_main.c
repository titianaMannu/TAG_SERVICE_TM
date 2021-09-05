/**
 * @file systbl_hack.c
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
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/version.h>

#include "systbl_hack.h"


/* Usage with Kernel >= 4.20 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#error "Required Kernel verison  >= 4.20."
#endif

#define MODNAME "SYSCALL TABLE HACKING SYSTEM"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tiziana Mannucci <titianamannucci@gmail.com>");
MODULE_DESCRIPTION("This Module implements a syscall table hacking system");
MODULE_INFO(name,
"systbl_hack");

/*must perform operation on the table atomically.*/
DEFINE_MUTEX(systbl_mtx);


int systbl_hack_init(void) {
    int ni_entries;
    printk(KERN_INFO "%s name = %s\n", MODNAME, THIS_MODULE->name);
    printk("%s: Initializing...\n", MODNAME);

    ni_entries = systbl_search();
    if (ni_entries == -1) {
        printk(KERN_INFO "%s : Failed initalization", MODNAME);
        return -1;
    }

    printk(KERN_INFO "%s : Found %d available entries... ready to hack.\n", MODNAME, ni_entries);
    printk(KERN_INFO "%s : module correctly mounted\n", MODNAME);
    return 0;
}

void systbl_hack_clean(void) {
    systbl_total_restore();
    printk(KERN_INFO "%s : Clean systable and exit.\n", MODNAME);
}

module_init(systbl_hack_init)
module_exit(systbl_hack_clean);