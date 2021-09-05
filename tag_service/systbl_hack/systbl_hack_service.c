/**
 * @file systbl_hack_service.c
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
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/moduleparam.h>
#include "systbl_hack.h"
#include "memory-mapper/virtual-to-phisical-memory-mapper.h"

#define MODNAME "SYSCALL TABLE HACKING SYSTEM"
//For security reasons this parameters are accessible by root and it is readable by users in his group
unsigned long sys_call_table = 0x0ULL;

module_param(sys_call_table, ulong, S_IRUSR | S_IRGRP);
MODULE_PARM_DESC(sys_call_table,
                 "virtual address of the syscall table.\n");

unsigned long sys_ni_syscall = 0x0ULL;

module_param(sys_ni_syscall, ulong, S_IRUSR | S_IRGRP);
MODULE_PARM_DESC(sys_ni_syscall,
                 "virtual address of the ni_syscall.\n");

void **sys_call_table_address;
void *sys_ni_syscall_address;
int table_status_map[TABLE_ENTRIES] __attribute__((aligned(128))) = {[0 ... TABLE_ENTRIES - 1] = 0};

int ni_free_positions[MAX_FREE_ENTRIES] = {[0 ... MAX_FREE_ENTRIES - 1] = 0};

module_param_array(ni_free_positions, int, NULL, 0660);
MODULE_PARM_DESC(ni_free_positions, "Array containing positions of free entries founded in the syscall table\n");

int free_entry_founded = 0;
unsigned long cr0;
extern struct mutex systbl_mtx;

module_param(free_entry_founded, int, S_IRUGO);
MODULE_PARM_DESC(nr_sysnis,
                 "Number of hackable entries in the syscall table.");

inline void safe_write_cr0(unsigned long cr0) {
    unsigned long __force_order;
    asm volatile(
    "mov %0, %%cr0"
    : "+r"(cr0), "+m"(__force_order)
    );
}


inline void enable_write_protection(void) {
    safe_write_cr0(cr0);
}

inline void disable_write_protection(void) {
    safe_write_cr0(cr0 & ~X86_CR0_WP);
}


int systbl_search(void) {
    void *current_page;
    for (current_page = KERNEL_START; current_page < KERNEL_END; current_page += _PAGE_SIZE) {
        if (page_table_walk((unsigned long) current_page) != NO_MAP &&
            match_pattern(current_page)) {
            AUDIT {
                printk(KERN_DEBUG "%s : found syscall table at %px\n", MODNAME, sys_call_table_address);
                printk(KERN_DEBUG "%s : found ni syscall at %px\n", MODNAME, sys_ni_syscall_address);
            }

            find_free_entries();
            return free_entry_founded;
        }
    }

    AUDIT printk(KERN_DEBUG "%s : research failed!\n", MODNAME);
    return -1;
}

EXPORT_SYMBOL(systbl_search);

int match_pattern(void *page) {
    void *next_page = page;
    void **test = page;
    unsigned long i = 0;

    for (; i < PAGE_SIZE; i += sizeof(void *)) {
        next_page = page + SEVENTH_NI_SYSCALL * sizeof(void *) + i;

        // If the table occupies 2 pages ;  the second one could be materialized in a frame
        if (
                ((unsigned long) (page + _PAGE_SIZE) == ((unsigned long) next_page & _ADDRESS_MASK))
                && page_table_walk((unsigned long) next_page) == NO_MAP
                )
            break;
        // go for patter matching
        test = page + i;
        if (
                (test[FIRST_NI_SYSCALL] != 0x0)
                && (((unsigned long) test[FIRST_NI_SYSCALL] & 0x3) == 0)
                && (test[FIRST_NI_SYSCALL] > KERNEL_START)
                && PATTERN(test)
                && (compatible(test))
                ) {

            sys_call_table_address = test;
            sys_call_table = (unsigned long) test;
            sys_ni_syscall_address = test[FIRST_NI_SYSCALL];
            sys_ni_syscall = (unsigned long) sys_ni_syscall_address;
            return 1;
        }
    }
    return 0;


}

int compatible(void **addr) {
    int index = 0;
    for (; index < FIRST_NI_SYSCALL; index++) {
        if (addr[index] == addr[FIRST_NI_SYSCALL]) return 0;
    }
    return 1;
}


int find_free_entries(void) {
    int i = FIRST_NI_SYSCALL;
    free_entry_founded = 0;
    for (; i < TABLE_ENTRIES; i++) {
        if (sys_call_table_address[i] == sys_ni_syscall_address &&
            free_entry_founded < MAX_FREE_ENTRIES) { //founded a ni_syscall
            AUDIT printk(KERN_DEBUG "%s : nisyscall index founded %d.\n", MODNAME, i);

            table_status_map[i] = 1; // set status of the entry to 1 (=FREE)
            ni_free_positions[free_entry_founded] = i; // report position founded
            free_entry_founded++; // later increment for correct indexing

        }
    }

    AUDIT printk(KERN_DEBUG "%s : number of free free entry founded %d.\n", MODNAME, free_entry_founded);
    return free_entry_founded;
}

int systbl_hack(void *new_syscall) {
    int index;
    int i;
    //must perform operation on the table atomically.
    mutex_lock(&systbl_mtx);

    if (free_entry_founded == 0) {

        mutex_unlock(&systbl_mtx);
        AUDIT printk(KERN_DEBUG "%s : there aren't entries to hack.\n", MODNAME);
        return -1;
    }
    //find first free hackable entry
    for (i = 0; i < free_entry_founded; i++) {

        index = ni_free_positions[i];

        if (table_status_map[index] == 1) {
            //here we are, insert a new syscall entry
            cr0 = read_cr0();
            disable_write_protection();

            sys_call_table_address[index] = new_syscall; //new system call insertion
            table_status_map[index] = 0; // set this entry as not available
            asm volatile ("sfence":: : "memory");

            enable_write_protection();
            mutex_unlock(&systbl_mtx); // release lock on the table
            AUDIT printk(KERN_DEBUG "%s : added a new syscall at %d and address of sysentry is %px\n", MODNAME, index,
                         sys_call_table_address[index]);


            return index;
        }
    }

    AUDIT printk(KERN_DEBUG "%s : System call insertion failed. Missing free entries.\n", MODNAME);
    mutex_unlock(&systbl_mtx); // release lock on the table
    return -1;

}

EXPORT_SYMBOL(systbl_hack);

int systbl_entry_restore(int index_restorable, int use_lock) {


    if (use_lock) mutex_lock(&systbl_mtx);

    if (index_restorable < 0 || !was_ni(index_restorable)) {
        if (use_lock) mutex_unlock(&systbl_mtx);
        AUDIT printk(KERN_DEBUG "%s : bad position\n", MODNAME);
        return -1;
    }

    if (!table_status_map[index_restorable]) {
        cr0 = read_cr0();
        disable_write_protection();

        sys_call_table_address[index_restorable] = sys_ni_syscall_address;
        table_status_map[index_restorable] = 1; //replace status of the entry to 1 (= FREE)
        asm volatile ("sfence":: : "memory");

        enable_write_protection();
        AUDIT printk(KERN_DEBUG "%s : restored entry %d\n", MODNAME, index_restorable);
        AUDIT printk(KERN_DEBUG "%s : address of %d entry of the table: %px\n", MODNAME, index_restorable,
                     sys_call_table_address[index_restorable]);
    }

    if (use_lock) mutex_unlock(&systbl_mtx);
    return 0;

}

EXPORT_SYMBOL(systbl_entry_restore);

void systbl_total_restore(void) {
    int i = 0, ni_index;

    mutex_lock(&systbl_mtx);
    for (; i < MAX_FREE_ENTRIES; i++) {
        ni_index = ni_free_positions[i];
        if (ni_index > 0) {
            //restore entry previously founded
            systbl_entry_restore(ni_index, 0); //lock already taken

            table_status_map[ni_index] = 0; // restore status map to the initial state
            AUDIT printk(KERN_DEBUG "%s : restoring %d entry.\n", MODNAME, ni_index);

            ni_free_positions[i] = 0; //delete knowledge of the entry
            free_entry_founded--;
        }

    }
    AUDIT printk(KERN_DEBUG "%s : System call table is restored to the initial state.\n", MODNAME);
    mutex_unlock(&systbl_mtx);
}

EXPORT_SYMBOL(systbl_total_restore);


int was_ni(int pos) {
    int i;
    for (i = 0; i < MAX_FREE_ENTRIES; i++) {
        //linear research
        if (ni_free_positions[i] == pos) {
            //founded
            return 1;
        }
    }

    return 0;
}