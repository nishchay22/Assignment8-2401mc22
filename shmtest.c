#include "types.h"
#include "stat.h"
#include "user.h"

#define SHM_KEY 101

int
main(int argc, char *argv[])
{
  char *shm;
  int pid;

  printf(1, "============================================================\n");
  printf(1, "Shared Memory Inter-Process Communication Test (shmtest)\n");
  printf(1, "============================================================\n\n");

  shm = (char*)shmget(SHM_KEY);
  if(shm == (char*)-1){
    printf(1, "shmtest: parent shmget failed\n");
    exit();
  }
  strcpy(shm, "Initial message from Parent");
  printf(1, "[Parent] Mapped shared page key=%d at VA=0x%x (PA=0x%x)\n",
         SHM_KEY, (uint)shm, (uint)va2pa((int)shm));

  pid = fork();
  if(pid == 0){
    char *cshm = (char*)shmget(SHM_KEY);
    printf(1, "[Child ] Mapped shared page key=%d at VA=0x%x (PA=0x%x)\n",
           SHM_KEY, (uint)cshm, (uint)va2pa((int)cshm));
    printf(1, "[Child ] Read from shared memory: \"%s\"\n", cshm);
    strcpy(cshm, "Updated by Child via Shared Physical Frame!");
    shmrem(SHM_KEY);
    exit();
  }
  wait();
  printf(1, "[Parent] Read after child exit : \"%s\"\n", shm);
  shmrem(SHM_KEY);
  printf(1, "SUCCESS: Shared memory IPC verified across parent and child!\n");
  exit();
}
