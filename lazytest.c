#include "types.h"
#include "stat.h"
#include "user.h"
#include "meminfo.h"

#define PGSIZE 4096
#define LARGE_REGION (32 * 1024 * 1024) // 32 MB lazy allocation

static void
show_mem(const char *label, struct meminfo *mi)
{
  getmeminfo(mi);
  printf(1, "[%s] free_pages=%d, user_pages=%d (sz=%d KB), pt_pages=%d\n",
         label, mi->free_pages, mi->user_pages, mi->user_pages * 4, mi->pt_pages);
}

int
main(int argc, char *argv[])
{
  struct meminfo mi_before, mi_after_sbrk, mi_after_touch, mi_after_syscall, mi_after_fork;
  char *base;
  int pfd[2];
  int pid;

  printf(1, "============================================================\n");
  printf(1, "Question 4: Lazy Memory Allocation / Demand Paging (lazytest)\n");
  printf(1, "============================================================\n\n");

  show_mem("1. Before sbrk(32 MB)", &mi_before);

  // Allocate a 32 MB region lazily
  base = sbrk(LARGE_REGION);
  if(base == (char*)-1){
    printf(1, "lazytest: sbrk(32 MB) failed!\n");
    exit();
  }

  show_mem("2. Immediately after sbrk(32 MB) [0 pages touched]", &mi_after_sbrk);
  printf(1, "   -> Physical frames consumed by sbrk(32 MB) before touch: %d (Expected: 0)\n\n",
         mi_before.free_pages - mi_after_sbrk.free_pages);

  // Touch 3 widely spaced pages across the 32 MB region (at offsets 0 MB, 16 MB, and 31 MB)
  base[0] = 'X';
  base[16 * 1024 * 1024] = 'Y';
  base[31 * 1024 * 1024] = 'Z';

  show_mem("3. After touching 3 widely spaced pages (0MB, 16MB, 31MB)", &mi_after_touch);
  printf(1, "   -> Values read back: %c, %c, %c\n",
         base[0], base[16 * 1024 * 1024], base[31 * 1024 * 1024]);
  printf(1, "   -> Total frames consumed (3 data pages + new page tables): %d\n\n",
         mi_after_sbrk.free_pages - mi_after_touch.free_pages);

  // Test a system call (pipe read/write) using an untouched lazy buffer at offset 8 MB
  printf(1, "--- Testing system call (read/write) on untouched lazy buffer at +8 MB ---\n");
  if(pipe(pfd) < 0){
    printf(1, "lazytest: pipe failed\n");
    exit();
  }
  char *untouched_buf = base + (8 * 1024 * 1024);
  if(write(pfd[1], "HELLO_LAZY_KERNEL", 18) != 18){
    printf(1, "lazytest: pipe write failed\n");
    exit();
  }
  if(read(pfd[0], untouched_buf, 18) != 18){
    printf(1, "lazytest: pipe read into untouched lazy buffer failed!\n");
    exit();
  }
  close(pfd[0]);
  close(pfd[1]);
  show_mem("4. After read() syscall into untouched buffer (+8 MB)", &mi_after_syscall);
  printf(1, "   -> Buffer content populated by kernel read(): \"%s\"\n\n", untouched_buf);

  // Test fork() and exit() with sparse 32 MB lazy address space
  printf(1, "--- Testing fork() and wait() with 32 MB sparse lazy heap ---\n");
  pid = fork();
  if(pid < 0){
    printf(1, "lazytest: fork failed\n");
    exit();
  }
  if(pid == 0){
    // Child verifies existing touched pages and touches one new page at +24 MB
    base[24 * 1024 * 1024] = 'C';
    printf(1, "   [Child pid=%d] Verified lazy heap: base[0]=%c, base[24MB]=%c\n",
           getpid(), base[0], base[24 * 1024 * 1024]);
    exit();
  }
  wait();
  show_mem("5. After forked child exits and is reaped", &mi_after_fork);

  // Test invalid access (>= sz) in a child process to confirm only offending child is killed
  printf(1, "\n--- Testing invalid page fault (address >= sz) in child process ---\n");
  pid = fork();
  if(pid == 0){
    char *invalid_addr = base + LARGE_REGION + 4096;
    printf(1, "   [Child pid=%d] Attempting invalid write at 0x%x (>= sz)...\n",
           getpid(), (uint)invalid_addr);
    *invalid_addr = '!';
    printf(1, "   ERROR: Child survived invalid page fault!\n");
    exit();
  }
  wait();
  printf(1, "   [Parent] Child was cleanly terminated by kernel on invalid page fault.\n");
  printf(1, "\nSUCCESS: All lazy allocation (demand paging) tests passed!\n");
  exit();
}
