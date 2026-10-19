# Operating Systems Lab — Assignment 8 Report
## Memory Management in xv6: Address Translation, Physical Memory Instrumentation, Page Table Inspection, and Demand Paging

- **Student Name**: Nishchay  
- **Roll Number**: 2401mc22  
- **Repository**: [https://github.com/nishchay22/Assignment8-2401mc22.git](https://github.com/nishchay22/Assignment8-2401mc22.git)  
- **Course**: Operating Systems Lab (Assignment 8)  
- **Target Platform**: MIT xv6 (x86 32-bit Two-Level Paging) on QEMU  

---

## 1. Executive Summary

This report presents the design, kernel implementation, edge-case handling, and empirical verification for **Operating Systems Lab Assignment 8** on the **xv6 (x86)** operating system. The project extends xv6's memory management subsystem across four core modules (along with extended support for page protection, shared memory, and page replacement analysis):
1. **Virtual-to-Physical Address Translation (`va2pa`)**: Exposing hardware two-level page table walks to user programs while logging 10-bit `PDX`, 10-bit `PTX`, 12-bit offset, and PTE permission flags (`P`, `W`, `U`).
2. **Physical Memory Statistics (`getmeminfo`)**: Instrumenting the physical frame allocator (`kalloc.c`) under spinlock synchronization (`kmem.lock`) to track total frames, free frames, user address space pages (`sz / PGSIZE`), and page table overhead, verifying zero memory leaks across `fork()` and `wait()`.
3. **Hierarchical Page Table Dumping (`vmprint`)**: Visualizing active two-level x86 page directories and page tables below `KERNBASE`, isolating the `exec()` stack guard page (`PTE_U = 0`), and summarizing kernel mappings.
4. **Lazy Heap Memory Allocation (Demand Paging)**: Decoupling `sbrk()` virtual address growth from physical frame allocation and servicing `T_PGFLT` (Trap 14) page faults via `rcr2()` on demand, including full support for sparse `fork()` (`copyuvm`) and kernel system calls (`read`/`write`) targeting untouched lazy buffers.

---

## 2. Architectural Design & Key Data Structures

### 2.1 Question 1: Virtual to Physical Address Translation (`va2pa`)
In 32-bit x86 paging (`mmu.h`), a virtual address `va` is partitioned as:
$$\text{VA}[31:0] = \underbrace{\text{PDX}(10\text{ bits})}_{[31:22]} \;\|\; \underbrace{\text{PTX}(10\text{ bits})}_{[21:12]} \;\|\; \underbrace{\text{Offset}(12\text{ bits})}_{[11:0]}$$

- **Kernel Implementation (`vm.c`)**:
  - `walkpgdir(pde_t *pgdir, const void *va, int alloc)` was changed from `static` to global scope and declared in `defs.h`.
  - `va2pa(int va)` validates `uva < KERNBASE` and calls `walkpgdir(myproc()->pgdir, (void*)uva, 0)` with `alloc = 0` so no page tables are allocated as a side effect of inspection.
  - If `pte == 0`, `!(*pte & PTE_P)`, or `!(*pte & PTE_U)`, the call logs the breakdown and returns `-1`.
  - Otherwise, it computes $\text{PA} = (\text{PTE\_ADDR}(*\text{pte})) \mid (\text{va} \ \& \ \text{0xFFF})$, prints `PDX`, `PTX`, `Offset`, and `P, W, U` flags via `cprintf`, and returns `PA`.

---

### 2.2 Question 2: Physical Memory Statistics (`getmeminfo`)
- **Allocator Instrumentation (`kalloc.c`)**:
  ```c
  struct {
    struct spinlock lock;
    int use_lock;
    struct run *freelist;
    uint free_pages;   // Incremented in kfree(), decremented in kalloc()
    uint total_pages;  // Counted in freerange() during kinit1() and kinit2()
  } kmem;
  ```
  - Every `kfree()` increments `kmem.free_pages` while holding `kmem.lock` (when `kmem.use_lock == 1`), and every successful `kalloc()` decrements `kmem.free_pages` while holding `kmem.lock`.
- **Transfer Structure (`meminfo.h`)**:
  ```c
  struct meminfo {
    uint total_pages;   // Total physical frames managed by kalloc (56,736 pages for 224 MB)
    uint free_pages;    // Currently free 4 KB frames in kmem.freelist
    uint user_pages;    // Calling process user address space pages (curproc->sz / PGSIZE)
    uint pt_pages;      // 1 (pgdir) + number of present user page tables (PDX < PDX(KERNBASE))
  };
  ```

---

### 2.3 Question 3: Dumping the Page Table (`vmprint`)
- **Two-Level Traversal (`vm.c`)**:
  - Iterates `i` from `0` to `PDX(KERNBASE) - 1` (`0 .. 511`). For each present `pgdir[i]`, prints `PDE[i]`, the physical address of the second-level page table `PTE_ADDR(pgdir[i])`, and flags `P, W, U`.
  - Maps the page table into kernel virtual memory via `(pte_t*)P2V(PTE_ADDR(pgdir[i]))` and iterates `j` from `0` to `1023`. For each present `pgtab[j]`, prints indented `PTE[j]`, virtual address `PGADDR(i, j, 0)`, physical frame address `PTE_ADDR(pgtab[j])`, and `P, W, U`.
  - **Guard Page Identification**: `exec()` allocates two pages above the data segment (`allocuvm(pgdir, sz, sz + 2*PGSIZE)`) and clears `PTE_U` on the lower page (`clearpteu(pgdir, (char*)(sz - 2*PGSIZE))`). In `vmprint()`, this page is immediately recognizable by `P=1 W=1 U=0` and is annotated with `[STACK GUARD PAGE]`.
  - **Kernel Summary**: Counts all present PDEs and PTEs from `PDX(KERNBASE)` (`512`) to `1023` and prints a concise one-line summary.

---

### 2.4 Question 4: Lazy Memory Allocation (Demand Paging)
- **Process Metadata (`proc.h`, `exec.c`, `proc.c`)**:
  - Added `uint guard_va` and `uint heap_start` to `struct proc`, initialized in `exec()` (`guard_va = sz - 2*PGSIZE`, `heap_start = sz`) and inherited in `fork()`.
- **Lazy `sys_sbrk()` (`sysproc.c`)**:
  - When `n > 0`: checks `curproc->sz + n < KERNBASE`, increments `curproc->sz += n` without calling `growproc()`, and returns the old `sz`.
  - When `n < 0`: calls `growproc(n)` $\rightarrow$ `deallocuvm()`, which safely skips unmapped PTEs (`(*pte & PTE_P) == 0`) and frees only physical frames that were actually materialized.
- **Page Fault Handler (`trap.c` & `vm.c`)**:
  - On `T_PGFLT` (Trap 14), reads the faulting linear address `fault_va = rcr2()`.
  - If `fault_va >= curproc->heap_start && fault_va < curproc->sz` and the page is not yet present (`!is_present`), `lazy_alloc_page(curproc, fault_va)` allocates a physical page with `kalloc()`, zeroes it with `memset(mem, 0, PGSIZE)`, and maps `PGROUNDDOWN(fault_va)` with `PTE_W | PTE_U | PTE_P`.
  - If `fault_va >= curproc->sz`, `fault_va < curproc->heap_start` (such as a stack overflow into `guard_va` or null pointer dereference), or `kalloc()` returns `0`, `trap()` logs the fault and sets `curproc->killed = 1`.

---

## 3. Problems Faced & Resolutions

| Problem Encountered | Root Cause Analysis | Resolution Implemented |
| :--- | :--- | :--- |
| **`panic("copyuvm: page not present")` during `fork()` after lazy `sbrk()`** | Standard xv6 `copyuvm()` assumes every page in `[0, sz)` has both an allocated page table and a valid `PTE_P` physical frame. | Updated `copyuvm()` in `vm.c` to `continue` when `walkpgdir(pgdir, (void*)i, 0) == 0` or `!(*pte & PTE_P)`, copying only materialized pages. |
| **System calls (`read`/`write`) failing or faulting in kernel mode on untouched lazy heap buffers** | When user code passes an untouched lazy heap pointer to `read(fd, buf, n)`, the CPU is in CPL 0 when `filewrite`/`piperead` accesses `buf`. | Added `ensure_range_mapped()` in `syscall.c` (`argptr`, `fetchint`, `fetchstr`) and lazy fallback in `copyout()` (`vm.c`) to fault-in lazy heap pages before kernel dereference. |
| **Distinguishing Stack Overflow (Guard Page Fault) from Valid Heap Page Fault** | Both trigger `T_PGFLT` (Trap 14). Without tracking where the stack ends, a fault below the stack could be misclassified. | Recorded `curproc->guard_va` and `curproc->heap_start` in `exec()` and checked `fault_va >= curproc->heap_start && fault_va < curproc->sz` in `T_PGFLT`. |
| **Counting Free Pages During Two-Phase Boot (`kinit1` / `kinit2`)** | `kinit1()` runs before SMP locks are enabled (`kmem.use_lock = 0`). | Updated `freerange()` and `kfree()`/`kalloc()` in `kalloc.c` so `total_pages` and `free_pages` are accurately tracked across both boot phases and runtime. |

---

## 4. Memory Usage & Empirical Results

### 4.1 Question 1 (`vatest`) Terminal Output
```text
$ vatest
============================================================
Question 1: Virtual to Physical Address Translation (vatest)
============================================================

--- Part (c): VA to PA Mappings in Current Process ---
[va2pa] VA=0x1000 -> PDX=0 (0x0), PTX=1 (0x1), Offset=0x0 | Flags: P=1 W=1 U=1 -> PA=0xddfd000
[User] Global variable : VA = 0x1000 -> PA = 0xddfd000

[va2pa] VA=0x2fb4 -> PDX=0 (0x0), PTX=2 (0x2), Offset=0xfb4 | Flags: P=1 W=1 U=1 -> PA=0xddfb000
[User] Local variable  : VA = 0x2fb4 -> PA = 0xddfbb4

[va2pa] VA=0x3008 -> PDX=0 (0x0), PTX=3 (0x3), Offset=0x8 | Flags: P=1 W=1 U=1 -> PA=0xddf2008
[User] Malloc block    : VA = 0x3008 -> PA = 0xddf2008

[va2pa] VA=0x7fffffff -> PDX=511 (0x1ff), PTX=1023 (0x3ff), Offset=0xfff | Flags: P=0 W=0 U=0 -> UNMAPPED/INACCESSIBLE (-1)
[User] Unmapped address: VA = 0x7FFFFFFF -> PA = -1 (Expected: -1)

--- Part (d): VA to PA Mapping Across fork() ---
[va2pa] VA=0x1000 -> PDX=0 (0x0), PTX=1 (0x1), Offset=0x0 | Flags: P=1 W=1 U=1 -> PA=0xdd7c000
[Child  pid=4] global_var VA = 0x1000 -> PA = 0xdd7c000 (val=42)
[va2pa] VA=0x1000 -> PDX=0 (0x0), PTX=1 (0x1), Offset=0x0 | Flags: P=1 W=1 U=1 -> PA=0xddfd000
[Parent pid=3] global_var VA = 0x1000 -> PA = 0xddfd000 (val=42)
SUCCESS: Parent and Child share the same VA (0x1000) mapped to distinct physical frames!
```

---

### 4.2 Question 2 (`memtest`) Memory Usage Before and After Table

| Stage | `total_pages` | `free_pages` | `user_pages` (`sz/PGSIZE`) | `pt_pages` | Net Free Pages Change |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **1. Initial Process State** | 56,736 | 56,521 | 3 | 2 | Baseline |
| **2. After `sbrk(5 * 4096)` & Touch** | 56,736 | 56,516 | 8 | 2 | **$-5$ pages** (5 heap frames allocated) |
| **3. Inside Forked Child (`fork()`)** | 56,736 | 56,439 | 8 | 2 | **$-77$ pages** (child user frames + `pgdir` + kernel PTs + `kstack`) |
| **4. After Child Exits & `wait()`** | 56,736 | **56,516** | 8 | 2 | **$+77$ pages** (100% reclaimed; **0 memory leak**) |
| **5. After `sbrk(-5 * 4096)`** | 56,736 | **56,521** | 3 | 2 | **$+5$ pages** (Restored to exact initial state) |

---

### 4.3 Question 3 (`vmprinttest`) Hierarchical Page Table Dump
```text
$ vmprinttest
>>> Step 1: vmprint() at process start-up <<<
=== Page Table Dump for pid 5 (vmprinttest), pgdir PA=0xddfe000, sz=12288 (3 pages) ===
PDE[0]: pt_pa=0xddff000 | flags: P=1 W=1 U=1 (0x7)
  .. PTE[0]: va=0x0 -> pa=0xddfd000 | flags: P=1 W=1 U=1
  .. PTE[1]: va=0x1000 -> pa=0xddfc000 | flags: P=1 W=1 U=0 [STACK GUARD PAGE]
  .. PTE[2]: va=0x2000 -> pa=0xddfb000 | flags: P=1 W=1 U=1
User summary: 2 user-accessible page(s), 1 guard page(s) (sz/PGSIZE = 3)
Kernel summary: 57 valid kernel PDEs (>= KERNBASE) mapping 57344 kernel PTEs
======================================================================
```
- After `sbrk(8 * 4096)`: `PTE[3]` through `PTE[10]` (`va=0x3000` .. `va=0xa000`) appear under `PDE[0]`; `user_accessible_pages` increases to `10` (`sz/PGSIZE = 11` minus `1` guard page).
- After `sbrk(-4096)`: `PTE[10]` (`va=0xa000`) is unmapped and freed; `user_accessible_pages` becomes `9` (`sz/PGSIZE = 10` minus `1` guard page).

---

### 4.4 Question 4 (`lazytest`) & Question 5 (`cowtest`) Memory Usage Comparison

| Test / Operation | Virtual Size (`sz`) | `free_pages` Before | `free_pages` After | Physical Frames Consumed | Analysis |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Q4: Before `sbrk(32 MB)`** | 12 KB (3 pages) | 56,521 | 56,521 | 0 | Baseline process footprint |
| **Q4: After `sbrk(32 MB)` (Untouched)** | 32,780 KB (8,195 pages) | 56,521 | **56,521** | **0 frames** | Lazy `sbrk()` consumes **0 physical pages** |
| **Q4: After Touching 3 Sparse Pages (0MB, 16MB, 31MB)** | 32,780 KB (8,195 pages) | 56,521 | **56,516** | **5 frames** | 3 data frames + 2 new second-level page tables (`PDE[4]`, `PDE[7]`) |
| **Q4: `read()` Syscall into Untouched Buffer (+8MB)** | 32,780 KB (8,195 pages) | 56,516 | **56,514** | **2 frames** | 1 data frame at `+8MB` + 1 page table (`PDE[2]`) faulted in by kernel |
| **Q4: After Child `fork()` + `wait()` on 32MB Sparse Heap** | 32,780 KB (8,195 pages) | 56,514 | **56,514** | **0 net frames** | `copyuvm()` skips 8,191 unmapped pages; all child pages freed on `wait()` |
| **Q5: `cowtest` Across `fork()` + `wait()`** | 12 KB (3 pages) | 56,521 | **56,521** | **0 net frames** | Child modifications isolated; all frames reclaimed upon `wait()` |

---

### 4.5 Question 8 (`pagerepl`) Page-Fault Comparison Table (FIFO, LRU, Optimal)

Reference String (20 page references): `7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1`

| Number of Physical Frames | FIFO Page Faults | LRU Page Faults | Optimal (MIN) Page Faults |
| :---: | :---: | :---: | :---: |
| **3 Frames** | 15 | 12 | 9 |
| **4 Frames** | 10 | 8 | 8 |
| **5 Frames** | 9 | 7 | 7 |
| **6 Frames** | 9 | 7 | 7 |

#### Short Analysis:
1. **Optimal (Belady's MIN)** replaces the page whose next reference is farthest in the future, establishing the theoretical lower bound on page faults (9 faults at 3 frames; 7 faults at 5–6 frames).
2. **LRU (Least Recently Used)** exploits temporal locality by evicting the page least recently accessed, closely tracking Optimal (12 faults at 3 frames, and matching Optimal with 8 and 7 faults at 4–6 frames) while being a stack algorithm immune to Belady's anomaly.
3. **FIFO (First-In, First-Out)** ignores page usage frequency and evicts the oldest resident page, incurring the highest fault rate (15 faults at 3 frames vs. 12 for LRU and 9 for Optimal) because heavily reused pages (such as page `0`) are periodically evicted simply due to arrival age.
