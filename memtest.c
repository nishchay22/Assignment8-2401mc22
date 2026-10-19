#include "types.h"
#include "stat.h"
#include "user.h"
#include "meminfo.h"

static void
print_meminfo(const char *stage, struct meminfo *mi)
{
  printf(1, "[%s]\n", stage);
  printf(1, "  total_pages : %d\n", mi->total_pages);
  printf(1, "  free_pages  : %d\n", mi->free_pages);
  printf(1, "  user_pages  : %d (sz / PGSIZE)\n", mi->user_pages);
  printf(1, "  pt_pages    : %d (page directory + user page tables)\n\n", mi->pt_pages);
}

int
main(int argc, char *argv[])
{
  struct meminfo mi_init, mi_grown, mi_parent_fork, mi_child, mi_after_reap;
  char *p;
  int i, pid;
  int num_new_pages = 5;

  printf(1, "============================================================\n");
  printf(1, "Question 2: Physical Memory Statistics (memtest)\n");
  printf(1, "============================================================\n\n");

  // 1. Initial memory statistics
  if(getmeminfo(&mi_init) < 0){
    printf(1, "memtest: getmeminfo failed\n");
    exit();
  }
  print_meminfo("1. Initial Process State", &mi_init);

  // 2. Grow memory by 5 pages (20 KB) using sbrk() and touch each page
  p = sbrk(num_new_pages * 4096);
  if(p == (char*)-1){
    printf(1, "memtest: sbrk failed\n");
    exit();
  }
  for(i = 0; i < num_new_pages; i++)
    p[i * 4096] = (char)(i + 1);

  getmeminfo(&mi_grown);
  print_meminfo("2. After Allocating & Touching 5 Pages via sbrk(5 * 4096)", &mi_grown);
  printf(1, "  -> Free pages decreased by: %d (Expected: %d)\n\n",
         mi_init.free_pages - mi_grown.free_pages, num_new_pages);

  // 3. Fork a child process and observe free_pages change
  pid = fork();
  if(pid < 0){
    printf(1, "memtest: fork failed\n");
    exit();
  }

  if(pid == 0){
    getmeminfo(&mi_child);
    print_meminfo("3. Inside Forked Child Process", &mi_child);
    printf(1, "  -> Free pages decreased during fork by: %d (child user pages + page tables + kstack)\n\n",
           mi_grown.free_pages - mi_child.free_pages);
    exit();
  } else {
    wait();
    getmeminfo(&mi_after_reap);
    print_meminfo("4. After Child Exits and is Reaped via wait()", &mi_after_reap);

    if(mi_after_reap.free_pages == mi_grown.free_pages){
      printf(1, "SUCCESS: No memory leak! free_pages returned to %d after child reap.\n",
             mi_after_reap.free_pages);
    } else {
      printf(1, "WARNING: free_pages before fork=%d, after reap=%d\n",
             mi_grown.free_pages, mi_after_reap.free_pages);
    }
  }

  // Shrink back the 5 heap pages and verify full restoration
  sbrk(-num_new_pages * 4096);
  getmeminfo(&mi_parent_fork);
  print_meminfo("5. After Shrinking Heap Back via sbrk(-5 * 4096)", &mi_parent_fork);

  exit();
}
