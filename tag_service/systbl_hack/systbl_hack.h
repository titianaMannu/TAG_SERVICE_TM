//
// Created by tiziana on 13/08/21.
//

#ifndef SOA_PROJECT_TM_SYSTBL_HACK_SERVICE_H

#define SOA_PROJECT_TM_SYSTBL_HACK_SERVICE_H
#define AUDIT if(1)
#define _PAGE_SIZE 4096
#define _ADDRESS_MASK 0xfffffffffffff000ULL

//Virtual address research start point
#define KERNEL_START ((void *)0xffffffff00000000ULL)
// end point
#define KERNEL_END   ((void *)0xfffffffffff00000ULL)


#define TABLE_ENTRIES       256
#define MAX_FREE_ENTRIES    15

#define FIRST_NI_SYSCALL    134
#define SECOND_NI_SYSCALL   174
#define THIRD_NI_SYSCALL    182
#define FOURTH_NI_SYSCALL   183
#define FIFTH_NI_SYSCALL    214
#define SIXTH_NI_SYSCALL    215
#define SEVENTH_NI_SYSCALL  236

#define PATTERN(addr)({ \
    int answer = 0;                        \
    if (                    \
    addr[FIRST_NI_SYSCALL] == addr[SECOND_NI_SYSCALL]                 \
    && (addr[FIRST_NI_SYSCALL] == addr[THIRD_NI_SYSCALL]) \
    && (addr[FIRST_NI_SYSCALL] == addr[FOURTH_NI_SYSCALL])\
    && (addr[FIRST_NI_SYSCALL] == addr[FIFTH_NI_SYSCALL])\
    && (addr[FIRST_NI_SYSCALL] == addr[SIXTH_NI_SYSCALL])\
    && (addr[FIRST_NI_SYSCALL] == addr[SEVENTH_NI_SYSCALL])    \
    )                       \
    answer = 1;            \
    \
    answer;                        \
})

int compatible(void **addr);

int match_pattern(void *page);

/**
 * @description Linear esearch of free entries in the system call table, setup of the state-map.
 * @return total number of the free entries founded
 */
int find_free_entries(void);

int was_ni(int pos);

/**
 * @description Insert a new system call by using the knowledge acquired with the previous research.
 * @param new_syscall  function pointer to insert
 * @return table index hacked on success, -1 on failure.
 */
int systbl_hack(void *new_syscall);

/**
 * @description restore the spcified table entry of the system call table.
 * @param index_restorable table index to restore
 * @param use_lock if 0 you MUST acquire the lock BEFORE calling this funtion.
 * @return 0 on success, -1 on failure.
 */
int systbl_entry_restore(int index_restorable, int use_lock);

/**
 * @description Research of the system call table address and of the free entries correesponding to the ni-syscall.
 * @return total number of the free entries founded, -1 on failure.
 */
int systbl_search(void);

/**
 * @description Restore the initial state (no knowledge at all)
 */
void systbl_total_restore(void);

#endif //SOA_PROJECT_TM_SYSTBL_HACK_SERVICE_H
