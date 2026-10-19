// System call numbers
#define SYS_fork        1
#define SYS_exit        2
#define SYS_wait        3
#define SYS_pipe        4
#define SYS_read        5
#define SYS_kill        6
#define SYS_exec        7
#define SYS_fstat       8
#define SYS_chdir       9
#define SYS_dup        10
#define SYS_getpid     11
#define SYS_sbrk       12
#define SYS_sleep      13
#define SYS_uptime     14
#define SYS_open       15
#define SYS_write      16
#define SYS_mknod      17
#define SYS_unlink     18
#define SYS_link       19
#define SYS_mkdir      20
#define SYS_close      21

// Assignment 8: Memory Management in xv6 system calls
#define SYS_va2pa       22  // Q1: Virtual to physical address translation
#define SYS_getmeminfo  23  // Q2: Physical memory and process page table statistics
#define SYS_vmprint     24  // Q3: Hierarchical dump of process page directory & page tables
#define SYS_mprotect    25  // Q6: Mark user pages read-only
#define SYS_munprotect  26  // Q6: Restore read-write permissions on user pages
#define SYS_shmget      27  // Q7: Map or create shared physical memory page by key
#define SYS_shmrem      28  // Q7: Unmap/release shared memory page by key
