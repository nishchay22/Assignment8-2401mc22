#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char *heap_base;
  int i;

  printf(1, "============================================================\n");
  printf(1, "Question 3: Dumping the Page Table of a Process (vmprinttest)\n");
  printf(1, "============================================================\n\n");

  // 1. Call vmprint() at start-up
  printf(1, ">>> Step 1: vmprint() at process start-up <<<\n");
  vmprint();

  // 2. Grow heap by 8 pages: sbrk(8 * 4096) and touch pages so PTEs are populated
  printf(1, "\n>>> Step 2: Growing heap with sbrk(8 * 4096) <<<\n");
  heap_base = sbrk(8 * 4096);
  if(heap_base == (char*)-1){
    printf(1, "vmprinttest: sbrk(8 * 4096) failed\n");
    exit();
  }
  for(i = 0; i < 8; i++){
    heap_base[i * 4096] = (char)('A' + i);
  }
  vmprint();

  // 3. Shrink heap by 1 page: sbrk(-4096)
  printf(1, "\n>>> Step 3: Shrinking heap with sbrk(-4096) <<<\n");
  if(sbrk(-4096) == (char*)-1){
    printf(1, "vmprinttest: sbrk(-4096) failed\n");
    exit();
  }
  vmprint();

  printf(1, "\nSUCCESS: vmprinttest completed all 3 page table dumps.\n");
  exit();
}
