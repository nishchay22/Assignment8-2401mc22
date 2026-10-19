#ifndef _MEMINFO_H_
#define _MEMINFO_H_

#include "types.h"

// Structure filled by getmeminfo(struct meminfo *mi) (Question 2)
struct meminfo {
  uint total_pages;   // Total physical pages managed by kalloc allocator
  uint free_pages;    // Currently free physical pages in kmem.freelist
  uint user_pages;    // Pages in calling process's user address space (sz / PGSIZE)
  uint pt_pages;      // Pages used by the calling process's page tables (pgdir + user PTE tables)
};

#endif // _MEMINFO_H_
