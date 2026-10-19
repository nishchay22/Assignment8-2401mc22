#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "meminfo.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

// Question 4(a): Lazy Memory Allocation for the Heap
// When n > 0, only increase myproc()->sz without calling growproc().
// Physical frames will be allocated on-demand in the T_PGFLT trap handler.
// When n < 0, shrink the address space and free any physical pages that were allocated.
int
sys_sbrk(void)
{
  int addr;
  int n;
  struct proc *curproc = myproc();

  if(argint(0, &n) < 0)
    return -1;
  addr = curproc->sz;
  if(n > 0){
    if(curproc->sz + (uint)n >= KERNBASE || curproc->sz + (uint)n < curproc->sz)
      return -1;
    curproc->sz += n;
  } else if(n < 0){
    if((uint)(-n) > curproc->sz)
      return -1;
    if(growproc(n) < 0)
      return -1;
  }
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Question 1: Virtual to Physical Address Translation
int
sys_va2pa(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return va2pa(va);
}

// Question 2: Physical Memory Statistics
int
sys_getmeminfo(void)
{
  struct meminfo *mi;
  if(argptr(0, (char**)&mi, sizeof(struct meminfo)) < 0)
    return -1;
  return getmeminfo(mi);
}

// Question 3: Dumping the Page Table of a Process
int
sys_vmprint(void)
{
  return vmprint();
}

// Question 6: Page Protection (mprotect / munprotect)
int
sys_mprotect(void)
{
  int addr, len;
  if(argint(0, &addr) < 0 || argint(1, &len) < 0)
    return -1;
  return mprotect((void*)addr, len);
}

int
sys_munprotect(void)
{
  int addr, len;
  if(argint(0, &addr) < 0 || argint(1, &len) < 0)
    return -1;
  return munprotect((void*)addr, len);
}

// Question 7: Shared Memory (shmget / shmrem)
int
sys_shmget(void)
{
  int key;
  if(argint(0, &key) < 0)
    return -1;
  return (int)shmget(key);
}

int
sys_shmrem(void)
{
  int key;
  if(argint(0, &key) < 0)
    return -1;
  return shmrem(key);
}
