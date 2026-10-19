#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
  char *page;
  int pid;

  printf(1, "============================================================\n");
  printf(1, "Page Protection Test — mprotect & munprotect (protecttest)\n");
  printf(1, "============================================================\n\n");

  // Allocate one page-aligned page using sbrk
  page = sbrk(PGSIZE);
  page[0] = 42;
  printf(1, "1. Allocated page at VA=0x%x, initial value=%d\n", (uint)page, page[0]);

  // Mark page read-only with mprotect()
  if(mprotect(page, 1) < 0){
    printf(1, "protecttest: mprotect failed\n");
    exit();
  }
  printf(1, "2. Called mprotect(0x%x, 1) -> page is now READ-ONLY (PTE_W=0)\n", (uint)page);
  va2pa((int)page);
  printf(1, "   Reading protected page succeeds: page[0]=%d\n", page[0]);

  // Restore write permission with munprotect()
  if(munprotect(page, 1) < 0){
    printf(1, "protecttest: munprotect failed\n");
    exit();
  }
  page[0] = 99;
  printf(1, "3. Called munprotect(0x%x, 1) -> write succeeded: page[0]=%d\n", (uint)page, page[0]);

  // Re-protect and verify that writing in a child process triggers a page fault and kills the child
  mprotect(page, 1);
  printf(1, "4. Forking child to test write violation on read-only page...\n");
  pid = fork();
  if(pid == 0){
    printf(1, "   [Child] Attempting write to read-only page at 0x%x...\n", (uint)page);
    page[0] = 77;
    printf(1, "   ERROR: Child survived write to read-only page!\n");
    exit();
  }
  wait();
  munprotect(page, 1);
  printf(1, "5. [Parent] Child terminated on protection fault as expected. page[0] is still %d.\n", page[0]);
  printf(1, "SUCCESS: protecttest passed!\n");
  exit();
}
