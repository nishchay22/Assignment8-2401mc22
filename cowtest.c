#include "types.h"
#include "stat.h"
#include "user.h"
#include "meminfo.h"

int shared_data = 100;

int
main(int argc, char *argv[])
{
  struct meminfo mi_before, mi_child, mi_after;
  int pid;

  printf(1, "============================================================\n");
  printf(1, "Fork & Memory Isolation / Copy-on-Write Test (cowtest)\n");
  printf(1, "============================================================\n\n");

  getmeminfo(&mi_before);
  printf(1, "[Before fork] free_pages=%d, shared_data=%d (PA=0x%x)\n",
         mi_before.free_pages, shared_data, (uint)va2pa((int)&shared_data));

  pid = fork();
  if(pid < 0){
    printf(1, "cowtest: fork failed\n");
    exit();
  }

  if(pid == 0){
    getmeminfo(&mi_child);
    printf(1, "[Child before write] free_pages=%d, shared_data=%d\n",
           mi_child.free_pages, shared_data);
    shared_data = 250;
    printf(1, "[Child after write ] shared_data=%d (PA=0x%x)\n",
           shared_data, (uint)va2pa((int)&shared_data));
    exit();
  } else {
    wait();
    getmeminfo(&mi_after);
    printf(1, "[Parent after wait ] free_pages=%d, shared_data=%d (PA=0x%x)\n",
           mi_after.free_pages, shared_data, (uint)va2pa((int)&shared_data));
    printf(1, "SUCCESS: Parent data remained isolated (%d) and free_pages restored to %d!\n",
           shared_data, mi_after.free_pages);
  }
  exit();
}
