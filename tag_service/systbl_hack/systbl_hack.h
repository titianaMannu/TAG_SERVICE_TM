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

int find_free_entries(void);

int was_ni(int pos);

int systbl_hack(void *new_syscall);

int systbl_entry_restore(int index_restorable, int use_lock);

int systbl_search(void);

void systbl_total_restore(void);

#endif //SOA_PROJECT_TM_SYSTBL_HACK_SERVICE_H
