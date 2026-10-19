# xv6 OS Lab Assignment 8 — Memory Management in xv6

**Author**: Nishchay  
**Roll Number**: 2401mc22  

This repository contains the complete kernel implementation, user test programs, and consolidated report for **Operating Systems Lab Assignment 8: Memory Management in xv6 (x86)**.

---

## Features Implemented

### Question 1: Virtual to Physical Address Translation (`va2pa`)
- Exposed `walkpgdir()` in `vm.c` via `defs.h`.
- Implemented system call `int va2pa(int va)` (`SYS_va2pa = 22`) that walks the two-level x86 page table of the calling process and returns the translated physical address (`PTE_ADDR(*pte) | PGOFF(va)`), or `-1` if unmapped (`!PTE_P`) or inaccessible from user space (`!PTE_U` or `>= KERNBASE`).
- Kernel diagnostic output via `cprintf` prints the 10-bit Page Directory Index (`PDX`), 10-bit Page Table Index (`PTX`), 12-bit Offset, and hardware PTE flag bits (`P`, `W`, `U`).
- User program [`vatest.c`](vatest.c): Translates a global variable, local stack variable, heap block (`malloc`), and unmapped address (`0x7FFFFFFF` $\rightarrow -1$), and demonstrates across `fork()` that parent and child share the same virtual address for `global_var` mapped to distinct physical frames.

### Question 2: Physical Memory Statistics (`getmeminfo`)
- Instrumented `kalloc.c` (`kinit1`, `kinit2`, `freerange`, `kalloc`, `kfree`) to track `total_pages` and `free_pages` under `kmem.lock`.
- Defined `struct meminfo` in [`meminfo.h`](meminfo.h) (`total_pages`, `free_pages`, `user_pages`, `pt_pages`).
- Implemented system call `int getmeminfo(struct meminfo *mi)` (`SYS_getmeminfo = 23`).
- User program [`memtest.c`](memtest.c): Prints memory statistics before and after growing the heap via `sbrk()`, across `fork()`, and verifies zero memory leaks after the child process exits and is reaped via `wait()`.

### Question 3: Dumping the Page Table of a Process (`vmprint`)
- Implemented system call `int vmprint(void)` (`SYS_vmprint = 24`) in `vm.c`.
- Hierarchically dumps all valid Page Directory Entries (`PDE`) and Page Table Entries (`PTE`) below `KERNBASE`, displaying indices, virtual addresses, physical frame addresses, and permission flags (`P`, `W`, `U`), explicitly identifying the stack guard page (`PTE_U = 0`) and summarizing kernel mappings.
- User program [`vmprinttest.c`](vmprinttest.c): Invokes `vmprint()` at startup, after `sbrk(8 * 4096)`, and after `sbrk(-4096)`, verifying that user-accessible pages equal `sz / PGSIZE - 1` (excluding the guard page).

### Question 4: Lazy Memory Allocation (Demand Paging for the Heap)
- Modified `sys_sbrk()` in `sysproc.c` so positive heap growth (`n > 0`) only advances `myproc()->sz` without eagerly allocating physical frames. Shrinking (`n < 0`) deallocates any mapped pages via `deallocuvm()`.
- Added `T_PGFLT` (Trap 14) handler in `trap.c` using `rcr2()` and `lazy_alloc_page()` in `vm.c` to allocate a zeroed 4 KB frame (`kalloc()`) and map it (`mappages()` with `PTE_W | PTE_U`) on demand when a heap page (`heap_start <= va < sz`) is touched.
- Handled all edge cases:
  - Out-of-memory in `kalloc()`, faults at `va >= sz`, and faults below the stack / on the stack guard page cleanly terminate the offending process without crashing the kernel.
  - Updated `copyuvm()` in `vm.c` to skip unmapped lazy PTEs during `fork()` instead of panicking.
  - Updated `argptr()`, `fetchint()`, `fetchstr()` in `syscall.c` and `copyout()` in `vm.c` so system calls like `read()`/`write()` into untouched lazy heap buffers fault in the backing frames transparently.
- User program [`lazytest.c`](lazytest.c): Allocates a 32 MB region via `sbrk()`, touches 3 sparse pages, tests `read()`/`write()` on an untouched lazy buffer, tests `fork()`/`wait()`, and verifies clean termination on an out-of-bounds fault.

### Additional Test Programs Included (Common Submission Requirements)
- [`cowtest.c`](cowtest.c): Verifies memory isolation and `free_pages` accounting across `fork()`.
- [`protecttest.c`](protecttest.c): Tests page-level read-only protection (`mprotect` / `munprotect`).
- [`shmtest.c`](shmtest.c): Tests shared physical memory mapping (`shmget` / `shmrem`) between parent and child processes.
- [`pagerepl.c`](pagerepl.c): Benchmarks **FIFO**, **LRU**, and **Optimal (Belady's MIN)** page replacement algorithms across 3–6 frame allocations.

---

## Modified & Added Files

| File | Role / Modifications |
| :--- | :--- |
| `vm.c`, `mmu.h`, `defs.h` | Exposed `walkpgdir` & `mappages`; implemented `va2pa`, `getmeminfo`, `vmprint`, `lazy_alloc_page`, non-panicking `copyuvm`, `mprotect`/`munprotect`, `shmget`/`shmrem`. |
| `kalloc.c` | Added `free_pages` and `total_pages` counters protected by `kmem.lock`. |
| `trap.c` | Added `T_PGFLT` (page fault) handler reading `rcr2()` for lazy heap allocation and invalid access termination. |
| `sysproc.c`, `syscall.c`, `syscall.h` | Registered system calls and modified `sys_sbrk()` for lazy allocation; updated `argptr`/`fetchstr` for untouched lazy buffers. |
| `proc.c`, `proc.h`, `exec.c` | Tracked `guard_va` and `heap_start` per process across `exec()` and `fork()`. |
| `meminfo.h`, `user.h`, `usys.S` | Defined `struct meminfo` and user-space system call stubs. |
| `vatest.c`, `memtest.c`, `vmprinttest.c`, `lazytest.c` | User test programs for Questions 1–4. |
| `cowtest.c`, `protecttest.c`, `shmtest.c`, `pagerepl.c` | Additional user test programs for Questions 5–8. |
| [`REPORT.md`](REPORT.md) | Consolidated 4-page technical report with design decisions, memory usage tables, and terminal outputs. |

---

## Build and Run Instructions

```bash
# Clean and build xv6 kernel and filesystem images
make clean
make

# Launch xv6 in headless QEMU
make qemu-nox

# Inside the xv6 shell ($):
$ vatest
$ memtest
$ vmprinttest
$ lazytest
$ cowtest
$ protecttest
$ shmtest
$ pagerepl
```

To exit QEMU: press `Ctrl-A` then `x`.
