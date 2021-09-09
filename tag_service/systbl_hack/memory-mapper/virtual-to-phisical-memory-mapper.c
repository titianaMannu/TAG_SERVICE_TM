/**
 * @file virtual-to-phisical-memory-mapper.c
 * @description Page table walk implementation inspired by VTPMO module provided during the SOA course.
 *
 * @author Tiziana Mannucci
 *
 * @mail titianamannucci@gmail.com
 *
 * @date 13/08/2021
 *
 *
 */

#include "virtual-to-phisical-memory-mapper.h"


#include <linux/module.h>
#include <linux/irqflags.h>
#include <linux/pgtable.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <asm/page.h>



static inline unsigned long read_content_cr3(void) {
    unsigned long cr3;
    asm volatile(
    "mov %%cr3,%0"
    :  "=r" (cr3)
    :
    );
    return cr3;
}


#define ADDR_MASK 0xfffffffffffff000ULL
#define PT_ADDR_MASK 0x7ffffffffffff000ULL
#define VALID 0x1ULL
#define LARGE_PAGE 0x80ULL

#define PAGE_WALK_AUDIT if(0)

#define LIB_NAME "PAGE_WALK"

#define PML4(vaddr) ((unsigned long long)(vaddr >> 39) & 0x1ffULL)
#define PDP(vaddr)  ((unsigned long long)(vaddr >> 30) & 0x1ffULL)
#define PDE(vaddr)  ((unsigned long long)(vaddr >> 21) & 0x1ffULL)
#define PTE(vaddr)  ((unsigned long long)(vaddr >> 12) & 0x1ffULL)


long page_table_walk(unsigned long vaddr) {

    pgd_t * pml4;
    pud_t *pdp;
    pmd_t *pde;
    pte_t *pte;

    long frame;


    pml4 = __va(read_content_cr3() & ADDR_MASK);
    if (!((pml4[PML4(vaddr)].pgd) & VALID)) {
        PAGE_WALK_AUDIT printk(KERN_INFO "%s: PML4 entry not mapped to physical memory.\n", LIB_NAME);
        return NO_MAP;
    }

    pdp = __va((pml4[PML4(vaddr)].pgd) & PT_ADDR_MASK);
    if (!((pdp[PDP(vaddr)].pud) & VALID)) {
        PAGE_WALK_AUDIT printk(KERN_INFO "%s: PDP entry not not mapped to physical memory.\n", LIB_NAME);
        return NO_MAP;
    } else if (unlikely((pdp[PDP(vaddr)].pud) & LARGE_PAGE)) {
        //1 GB page case
        frame = ((pdp[PDP(vaddr)].pud) & PT_ADDR_MASK) >> 30;
        return frame;
    }


    pde = __va((pdp[PDP(vaddr)].pud) & PT_ADDR_MASK);
    if (!((pde[PDE(vaddr)].pmd) & VALID)) {
        PAGE_WALK_AUDIT printk(KERN_INFO "%s: PDE entry not not mapped to physical memory.\n", LIB_NAME);
        return NO_MAP;
    }

    if (unlikely((pde[PDE(vaddr)].pmd) & LARGE_PAGE)) {
        PAGE_WALK_AUDIT printk(KERN_INFO "%s: PDE region maps large page 2 MB.\n", LIB_NAME);
        frame = ((pde[PDE(vaddr)].pmd) & PT_ADDR_MASK) >> 21;
        //stop walking and return large page physical address
        return frame;
    }


    pte = __va((pde[PDE(vaddr)].pmd) & PT_ADDR_MASK);
    if (!((pte[PTE(vaddr)].pte) & VALID)) {
        PAGE_WALK_AUDIT printk(KERN_DEBUG "%s: PTE page not mapped to physical memory.\n", LIB_NAME);
        return NO_MAP;
    }

    frame = ((pte[PTE(vaddr)].pte) & PT_ADDR_MASK) >> 12;
    PAGE_WALK_AUDIT printk(KERN_DEBUG "%s: Found mapping at frame: %ld.\n", LIB_NAME, frame);

    return frame;
}
