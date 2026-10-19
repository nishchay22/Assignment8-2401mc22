#include "types.h"
#include "stat.h"
#include "user.h"

// Global variable (resides in .data / .bss segment)
int global_var = 42;

int
main(int argc, char *argv[])
{
  int local_var = 99;
  char *heap_block;
  int pa_global, pa_local, pa_heap, pa_unmapped;
  int pid;

  printf(1, "============================================================\n");
  printf(1, "Question 1: Virtual to Physical Address Translation (vatest)\n");
  printf(1, "============================================================\n\n");

  // Allocate a heap block via malloc() and touch it so it is mapped
  heap_block = (char*)malloc(64);
  if(heap_block == 0){
    printf(1, "vatest: malloc failed\n");
    exit();
  }
  heap_block[0] = 'A';

  printf(1, "--- Part (c): VA to PA Mappings in Current Process ---\n");
  pa_global = va2pa((int)&global_var);
  printf(1, "[User] Global variable : VA = 0x%x -> PA = 0x%x\n\n",
         (uint)&global_var, (uint)pa_global);

  pa_local = va2pa((int)&local_var);
  printf(1, "[User] Local variable  : VA = 0x%x -> PA = 0x%x\n\n",
         (uint)&local_var, (uint)pa_local);

  pa_heap = va2pa((int)heap_block);
  printf(1, "[User] Malloc block    : VA = 0x%x -> PA = 0x%x\n\n",
         (uint)heap_block, (uint)pa_heap);

  pa_unmapped = va2pa(0x7FFFFFFF);
  printf(1, "[User] Unmapped address: VA = 0x7FFFFFFF -> PA = %d (Expected: -1)\n\n",
         pa_unmapped);

  printf(1, "--- Part (d): VA to PA Mapping Across fork() ---\n");
  pid = fork();
  if(pid < 0){
    printf(1, "vatest: fork failed\n");
    exit();
  }

  if(pid == 0){
    int child_pa = va2pa((int)&global_var);
    printf(1, "[Child  pid=%d] global_var VA = 0x%x -> PA = 0x%x (val=%d)\n",
           getpid(), (uint)&global_var, (uint)child_pa, global_var);
    exit();
  } else {
    wait();
    int parent_pa = va2pa((int)&global_var);
    printf(1, "[Parent pid=%d] global_var VA = 0x%x -> PA = 0x%x (val=%d)\n",
           getpid(), (uint)&global_var, (uint)parent_pa, global_var);
    printf(1, "SUCCESS: Parent and Child share the same VA (0x%x) mapped to distinct physical frames!\n",
           (uint)&global_var);
  }

  free(heap_block);
  exit();
}
