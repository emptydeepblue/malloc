# ft_malloc — Complete Documentation

A from-scratch implementation of the C dynamic memory allocator: `malloc`, `free`,
`realloc`, plus a debug helper `show_alloc_mem`. It talks **directly to the kernel**
through `mmap(2)` / `munmap(2)` — it never uses the libc allocator underneath — and
is **thread-safe** via a POSIX mutex.

This document is written to be read top-to-bottom by someone who has never written an
allocator. It first builds up the concepts you need, then walks through the actual code.

---

## Table of contents

1. [What problem does `malloc` solve?](#1-what-problem-does-malloc-solve)
2. [Background concepts you need](#2-background-concepts-you-need)
   - [The process address space](#21-the-process-address-space)
   - [Virtual memory and pages](#22-virtual-memory-and-pages)
   - [`mmap` / `munmap`: getting memory from the kernel](#23-mmap--munmap-getting-memory-from-the-kernel)
   - [Why not just `mmap` every allocation?](#24-why-not-just-mmap-every-allocation)
   - [Memory alignment](#25-memory-alignment)
   - [Block headers (intrusive metadata)](#26-block-headers-intrusive-metadata)
   - [Fragmentation, splitting, coalescing](#27-fragmentation-splitting-coalescing)
   - [Thread safety](#28-thread-safety)
3. [Architecture of ft_malloc](#3-architecture-of-ft_malloc)
   - [Zones](#31-zones)
   - [Blocks](#32-blocks)
   - [The global state](#33-the-global-state)
   - [Memory layout of a zone](#34-memory-layout-of-a-zone)
4. [The public API](#4-the-public-api)
5. [Code walkthrough](#5-code-walkthrough)
   - [`malloc`](#51-malloc)
   - [`align_size`](#52-align_size)
   - [`find_or_create_zone` / `create_zone`](#53-find_or_create_zone--create_zone)
   - [`find_suitable_block` / `split_block`](#54-find_suitable_block--split_block)
   - [`free`](#55-free)
   - [`merge_adjacent_free_blocks` (coalescing)](#56-merge_adjacent_free_blocks-coalescing)
   - [`realloc`](#57-realloc)
   - [`show_alloc_mem`](#58-show_alloc_mem)
6. [Building, using, testing](#6-building-using-testing)
7. [Design trade-offs & known limitations](#7-design-trade-offs--known-limitations)
8. [Glossary](#8-glossary)
9. [References](#9-references)

---

## 1. What problem does `malloc` solve?

A program needs memory whose size or lifetime isn't known at compile time — a string the
user types, a list that grows, a buffer sized at runtime. That memory can't live on the
**stack** (which is fixed-frame and freed when a function returns). It must come from the
**heap**: a region you can grow, carve up, and hand back on demand.

The job of an allocator is to sit between your program and the kernel:

- The **kernel** deals in large, page-sized chunks of memory and charges a system call
  for every request. It has no idea you wanted "37 bytes for a name."
- Your **program** asks for arbitrary small sizes, thousands of times, and expects each
  call to be cheap.

`malloc` bridges that gap: it requests big chunks from the kernel rarely, then sub-divides
them into the small pieces your program asks for, tracks which pieces are in use, and
recycles freed pieces for future requests.

The four functions implemented here:

| Function | Contract |
|----------|----------|
| `void *malloc(size_t size)` | Return a pointer to at least `size` usable bytes, suitably aligned, or `NULL` on failure. |
| `void free(void *ptr)` | Return a block previously returned by `malloc`/`realloc` to the allocator. `free(NULL)` is a no-op. |
| `void *realloc(void *ptr, size_t size)` | Resize an existing allocation, preserving its contents, possibly moving it. |
| `void show_alloc_mem(void)` | Debug helper: print every live allocation, grouped by zone. |

---

## 2. Background concepts you need

### 2.1 The process address space

Every process sees its own private, linear range of addresses (on x86-64, a 48-bit
canonical space). Conceptually, from low to high addresses:

```
  high ┌──────────────────────────┐
       │   stack  (grows down ↓)   │   local variables, call frames
       │            …             │
       │                          │
       │   mmap region            │   shared libs, big malloc blocks, OUR ZONES
       │            …             │
       │   heap   (grows up ↑)    │   classic brk/sbrk heap
       ├──────────────────────────┤
       │   BSS / data             │   globals
       │   text                   │   program code
   low └──────────────────────────┘
```

Two classic ways to get heap memory from the kernel:

- **`brk` / `sbrk`** — move the single "program break" up or down. Simple but one
  contiguous region; freeing memory in the middle can't be returned to the OS.
- **`mmap`** — map independent regions anywhere in the address space, and unmap them
  individually.

**This project uses `mmap`/`munmap` exclusively** — no `brk`/`sbrk`. That makes it easy to
hand whole regions back to the OS (important for large allocations).

### 2.2 Virtual memory and pages

The addresses your program uses are **virtual**. The CPU's MMU translates them to physical
RAM through page tables, in fixed-size units called **pages** (typically **4096 bytes** on
Linux — query it with `sysconf(_SC_PAGESIZE)` or `getpagesize()`).

Consequences that matter to an allocator:

- The kernel only ever gives out memory in **whole-page multiples**. Asking for 100 bytes
  via `mmap` still consumes a full 4 KB page.
- A freshly `mmap`'d anonymous region is **zero-filled and lazily backed**: physical RAM
  isn't actually committed until you touch each page. So reserving a 4 MB zone is cheap
  until you write into it.

### 2.3 `mmap` / `munmap`: getting memory from the kernel

```c
void *mmap(NULL, length, PROT_READ | PROT_WRITE,
           MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
```

- `NULL` — let the kernel pick the address.
- `length` — rounded up to a page multiple by the kernel.
- `PROT_READ | PROT_WRITE` — we want to read and write the memory.
- `MAP_ANONYMOUS` — not backed by a file; it's just RAM. (So the fd is `-1`, offset `0`.)
- `MAP_PRIVATE` — copy-on-write, private to this process.
- Returns the base address, or `MAP_FAILED` ( `(void*)-1` ) on error — **always check.**

`munmap(addr, length)` returns the region to the kernel. After it, touching that memory
segfaults.

### 2.4 Why not just `mmap` every allocation?

Because system calls are **expensive** (a user→kernel transition, page-table work, TLB
effects), and because every `mmap` rounds up to a full page. `malloc(16)` backed by its
own `mmap` would waste ~4080 bytes and cost a syscall every time.

The fix, used by essentially every real allocator (glibc, jemalloc, TCMalloc) and by this
one: **`mmap` a large region once, then sub-allocate many small blocks out of it in user
space** with no further syscalls. Only large or whole-region operations hit the kernel.

### 2.5 Memory alignment

**Alignment** means an object's address is a multiple of some boundary (8, 16, …). CPUs
load/store most efficiently — and on some architectures *only correctly* — when data is
aligned to its size. The C standard requires `malloc` to return memory aligned for **any**
type, i.e. to `alignof(max_align_t)`, which is **16 bytes** on x86-64 Linux.

This project enforces 16-byte alignment in three places (see
[§5.2](#52-align_size) and the alignment note in `docs/notes/`):

1. Every requested size is rounded **up** to a multiple of 16 (`align_size`).
2. The first block inside a zone is placed on a 16-byte boundary.
3. Because both the block start and every block size are 16-multiples, and the header
   `t_block` is itself a 16-multiple in size, every returned pointer stays aligned.

The rounding trick:

```c
(size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1)   // ALIGNMENT is a power of two (16)
```

Adding `ALIGNMENT-1` pushes you up to (or past) the next boundary; the `& ~(ALIGNMENT-1)`
mask clears the low bits, snapping back down to the boundary. E.g. `1→16`, `16→16`,
`17→32`.

### 2.6 Block headers (intrusive metadata)

When `free(ptr)` is called, the allocator gets only `ptr` — it must figure out how big
that allocation was and where it sits. The standard technique is an **intrusive header**:
immediately *before* the bytes handed to the user, the allocator stores bookkeeping.

```
            ┌──────────────┬───────────────────────────────┐
            │  t_block     │  user data (size bytes)        │
            └──────────────┴───────────────────────────────┘
            ^              ^
        block header   pointer returned to the user (ptr)
```

So:

- `malloc` returns `(char*)block + sizeof(t_block)`.
- `free(ptr)` recovers the header with `(t_block*)((char*)ptr - sizeof(t_block))`.

The header here (`t_block`) stores the block size, a free/used flag, and `prev`/`next`
pointers linking it to its neighbors *within the same zone* (an intrusive doubly-linked
list). This is what makes splitting and coalescing O(1).

### 2.7 Fragmentation, splitting, coalescing

- **Splitting** — when a free block is bigger than the request, carve off exactly what's
  needed and turn the remainder into a new, smaller free block. Avoids wasting the tail.
- **Coalescing (merging)** — when a block is freed, if its immediate neighbors are also
  free, fuse them into one larger free block. Without this you get **external
  fragmentation**: lots of free memory, but chopped into pieces too small to satisfy a
  request.
- **Internal fragmentation** — wasted space *inside* a block, e.g. the padding from
  alignment rounding (asking for 17 but getting a 32-byte block).

This allocator does both splitting (on `malloc`/`realloc`) and coalescing (on `free`).

### 2.8 Thread safety

If two threads call `malloc` at the same time, they both walk and mutate the **same**
global zone list. Interleaved updates corrupt pointers, double-allocate a block, or leak
it. The C library `malloc` is required to be thread-safe.

The simplest correct solution — used here — is a single global **mutex**
(`pthread_mutex_t`). Every public entry point locks it on the way in and unlocks on the
way out, so only one thread is ever inside the allocator at a time. This serializes the
allocator (a throughput bottleneck under heavy contention) but is simple and provably
correct. See [§7](#7-design-trade-offs--known-limitations) for the trade-off.

---

## 3. Architecture of ft_malloc

The allocator classifies every request into one of **three size classes**, each backed by
a different zone strategy. This is the central design idea (borrowed from real allocators'
"size classes").

| Class | Request size        | Backing strategy                                   |
|-------|---------------------|----------------------------------------------------|
| TINY  | `≤ 64` bytes        | Sub-allocated from shared **4 MB** pre-mapped zones |
| SMALL | `65 … 1024` bytes   | Sub-allocated from shared **8 MB** pre-mapped zones |
| LARGE | `> 1024` bytes      | Its **own** `mmap`'d zone, sized to the request     |

Defined in [`inc/ft_malloc.h`](../inc/ft_malloc.h):

```c
#define ALIGNMENT 16
#define TINY_MAX 64
#define SMALL_MAX 1024
#define TINY_ZONE_SIZE  (4 * 1024 * 1024)   // 4 MB
#define SMALL_ZONE_SIZE (8 * 1024 * 1024)   // 8 MB
```

**Why size classes?** Grouping similarly-sized allocations into shared pools means a TINY
zone amortizes one `mmap` over thousands of tiny allocations, and keeps tiny and large
objects from fragmenting each other. LARGE requests are rare and big, so giving each its
own region (and `munmap`-ing it the moment it's freed) returns memory to the OS promptly.

### 3.1 Zones

A **zone** is one `mmap`'d region. TINY/SMALL zones are pools shared by many allocations;
LARGE zones hold exactly one allocation. All zones live in a single global singly-linked
list, `g_zones`.

```c
typedef enum e_zone_type { TINY, SMALL, LARGE } t_zone_type;

typedef struct s_zone {
    t_zone_type    type;    // TINY / SMALL / LARGE
    size_t         size;    // total bytes mmap'd for this zone
    void          *start;   // base address returned by mmap (for munmap)
    t_block       *blocks;  // head of this zone's intrusive block list
    struct s_zone *next;    // next zone in the global list
} t_zone;
```

### 3.2 Blocks

A **block** is one sub-division of a zone — either a live allocation or a free hole. Blocks
in a zone form an intrusive **doubly-linked list** (ordered by address), which is what lets
`split_block` and `merge_adjacent_free_blocks` work in O(1).

```c
typedef struct s_block {
    size_t          size;   // usable bytes after this header
    int             free;   // 1 = free, 0 = in use
    struct s_block *prev;    // previous block in the zone (lower address)
    struct s_block *next;    // next block in the zone (higher address)
} t_block;
```

### 3.3 The global state

The subject permits exactly two globals — one for the allocations, one for the lock —
defined in [`src/malloc.c`](../src/malloc.c):

```c
t_zone         *g_zones = NULL;                       // head of the zone list
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;  // serializes the allocator
```

`PTHREAD_MUTEX_INITIALIZER` initializes the mutex statically, so there's no runtime setup
and no init order problem when the library is `LD_PRELOAD`'d before `main`.

### 3.4 Memory layout of a zone

A single TINY/SMALL zone right after creation, with one big free block:

```
mmap'd region (e.g. 4 MB), page-aligned
┌──────────────┬─pad─┬──────────────┬─────────────────────────────────────────┐
│  t_zone      │ …   │  t_block     │  free space (one big free block)         │
│  (header)    │     │  (header)    │                                          │
└──────────────┴─────┴──────────────┴─────────────────────────────────────────┘
^                    ^              ^
zone->start          zone->blocks   first user pointer would start here
(page aligned)       (16-aligned)
```

`pad` is the few bytes inserted so that `zone->blocks` lands on a 16-byte boundary even
though `sizeof(t_zone)` might not be a multiple of 16.

After a couple of `malloc`s have split the big block:

```
┌────────┬─────────┬───────┬─────────┬───────┬───────────────────────────┐
│ t_zone │ t_block │ used  │ t_block │ used  │ t_block │ remaining free    │
└────────┴─────────┴───────┴─────────┴───────┴───────────────────────────┘
                    └ alloc A         └ alloc B          └ tail free block
```

Each `t_block` header sits immediately before the bytes it describes; `next`/`prev` chain
them in address order.

---

## 4. The public API

```c
#include "ft_malloc.h"   // pulls in <sys/mman.h>, <pthread.h>, <stdint.h>, libft

void  *malloc(size_t size);
void   free(void *ptr);
void  *realloc(void *ptr, size_t size);
void   show_alloc_mem(void);
```

Behavioral contract (matching libc where it matters):

- `malloc(0)` returns `NULL` in this implementation. *(Note: libc may return a unique
  freeable pointer instead; see [§7](#7-design-trade-offs--known-limitations).)*
- `free(NULL)` does nothing.
- `realloc(NULL, n)` is equivalent to `malloc(n)`.
- `realloc(ptr, 0)` frees `ptr` and returns `NULL`.
- All returned pointers are 16-byte aligned.

---

## 5. Code walkthrough

### 5.1 `malloc`

[`src/malloc.c`](../src/malloc.c)

```c
void *malloc(size_t size) {
    if (size == 0) return (NULL);
    t_zone_type zone_type;
    t_zone     *zone;
    t_block    *block;

    pthread_mutex_lock(&g_mutex);                 // 1. enter critical section
    size = align_size(size);                       // 2. round up to 16
    zone_type = (size <= TINY_MAX) ? TINY
              : (size <= SMALL_MAX) ? SMALL : LARGE;  // 3. classify
    zone = find_or_create_zone(zone_type, size);   // 4. get a zone that can hold it
    if (!zone) { pthread_mutex_unlock(&g_mutex); return (NULL); }

    if (zone_type == LARGE) block = zone->blocks;  // 5. LARGE = the whole zone is the block
    else block = find_suitable_block(zone, size);  //    else first-fit search
    if (!block) { pthread_mutex_unlock(&g_mutex); return (NULL); }

    if (zone_type != LARGE) split_block(block, size); // 6. trim the tail off
    block->free = 0;                                  // 7. mark it used
    pthread_mutex_unlock(&g_mutex);                   // 8. leave critical section
    return ((void*)((char *)block + sizeof(t_block))); // 9. skip header → user ptr
}
```

Step by step:

1. **Lock** — serialize against other threads.
2. **Align** the request up to a 16-byte multiple.
3. **Classify** into TINY / SMALL / LARGE by comparing against `TINY_MAX` / `SMALL_MAX`.
4. **Find or create a zone** able to satisfy the request.
5. For **LARGE**, the zone *is* a single block. Otherwise do a **first-fit** search for a
   free block big enough.
6. **Split** the chosen block so the leftover tail becomes a new free block (TINY/SMALL
   only — a LARGE zone is exactly sized).
7. **Mark** the block in use.
8. **Unlock.**
9. Return the address **after** the header — that's the user's memory.

### 5.2 `align_size`

```c
size_t align_size(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}
```

Rounds `size` up to the next multiple of 16 (see [§2.5](#25-memory-alignment)). Guarantees
that block sizes — and therefore the address of the *next* block after a split — stay
16-aligned.

### 5.3 `find_or_create_zone` / `create_zone`

```c
t_zone *find_or_create_zone(t_zone_type type, size_t size) {
    if (type == LARGE) return create_zone(type, size);   // LARGE: always fresh
    t_zone  *current = g_zones;
    t_block *block;
    while (current) {                                     // scan existing zones
        if (current->type == type) {
            block = find_suitable_block(current, size);   // room in this pool?
            if (block) return (current);
        }
        current = current->next;
    }
    return (create_zone(type, size));                    // none had room → new pool
}
```

For TINY/SMALL it reuses an existing pool that still has a fitting free block; only when
none does (or there are none yet) does it `mmap` a new pool. LARGE always allocates a fresh
zone sized to the request.

```c
t_zone *create_zone(t_zone_type type, size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t zone_size = (type == TINY)  ? TINY_ZONE_SIZE
                     : (type == SMALL) ? SMALL_ZONE_SIZE
                     // LARGE: round (request + headers) up to a whole number of pages
                     : ((size + sizeof(t_zone) + sizeof(t_block) + page_size - 1)
                        / page_size) * page_size;

    void *zone_start = mmap(NULL, zone_size, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (zone_start == MAP_FAILED) return (NULL);          // always check mmap

    t_zone   *new_zone   = (t_zone*)zone_start;           // zone header at the base
    uintptr_t blocks_addr = (uintptr_t)(zone_start + sizeof(t_zone));
    new_zone->type  = type;
    new_zone->size  = zone_size;
    new_zone->start = zone_start;
    blocks_addr = (blocks_addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1); // align first block
    new_zone->blocks = (t_block*)blocks_addr;

    if (type == LARGE) {                                   // one exact, already-used block
        new_zone->blocks->size = size;
        new_zone->blocks->free = 0;
        new_zone->blocks->prev = NULL;
    } else {                                               // one big free block spanning the pool
        size_t available = zone_size - (blocks_addr - (uintptr_t)zone_start) - sizeof(t_block);
        new_zone->blocks->size = available;
        new_zone->blocks->free = 1;
        new_zone->blocks->prev = NULL;
    }
    new_zone->blocks->next = NULL;

    new_zone->next = g_zones;   // push onto the global list (prepend = O(1))
    g_zones = new_zone;
    return (new_zone);
}
```

Key points:

- The **zone header lives inside its own mapping**, at the base address — no separate
  allocation for metadata.
- The first block is **16-aligned** even though the zone header may not end on a 16
  boundary.
- A TINY/SMALL zone starts life as **one big free block** covering all usable space;
  subsequent `malloc`s carve it up via `split_block`.
- A LARGE zone is created with its single block **already marked used** and sized exactly
  to the (aligned) request.

### 5.4 `find_suitable_block` / `split_block`

```c
t_block *find_suitable_block(t_zone *zone, size_t size) {
    t_block *current = zone->blocks;
    while (current) {
        if (current->free && current->size >= size) return (current); // first-fit
        current = current->next;
    }
    return (NULL);
}
```

**First-fit**: return the first free block large enough. Simple and fast; can fragment more
than best-fit, but coalescing on free keeps it in check.

```c
void split_block(t_block *block, size_t size) {
    // Only split if the leftover can hold a header + at least one aligned unit.
    if (block->size >= size + sizeof(t_block) + ALIGNMENT) {
        t_block *new_block = (t_block*)((char*)block + sizeof(t_block) + size);
        new_block->size = block->size - size - sizeof(t_block);
        new_block->free = 1;
        new_block->prev = block;
        new_block->next = block->next;
        if (block->next) block->next->prev = new_block;
        block->size = size;          // shrink the original to the requested size
        block->next = new_block;     // link the new free remainder in
    }
}
```

If the chosen free block is much bigger than needed, carve off the request and turn the
**remainder into a new free block**, threaded into the doubly-linked list. The guard
`block->size >= size + sizeof(t_block) + ALIGNMENT` means: only split if the leftover is
big enough to be a *useful* free block (its own header plus at least 16 usable bytes).
Otherwise leave the slight excess inside the allocated block (a little internal
fragmentation, which is cheaper than tracking a useless sliver).

### 5.5 `free`

[`src/free.c`](../src/free.c)

```c
void free(void *ptr) {
    if (!ptr) return;                                   // free(NULL) is a no-op
    pthread_mutex_lock(&g_mutex);
    t_block *block = (t_block*)((char*)ptr - sizeof(t_block)); // recover header
    t_zone  *zone  = find_zone_containing_block(block);        // which zone owns it?
    if (!zone) { pthread_mutex_unlock(&g_mutex); return; }     // unknown ptr → ignore
    block->free = 1;                                    // mark free
    merge_adjacent_free_blocks(block);                  // coalesce neighbors
    if (zone->type == LARGE && block->free) {           // LARGE: return whole zone to OS
        if (g_zones == zone) {
            g_zones = zone->next;                       // unlink from head
        } else {
            t_zone *prev = g_zones;                     // …or from the middle
            while (prev && prev->next != zone) prev = prev->next;
            if (prev) prev->next = zone->next;
        }
        munmap(zone->start, zone->size);                // hand pages back to the kernel
    }
    pthread_mutex_unlock(&g_mutex);
}
```

- The header is recovered by pointer arithmetic (`ptr - sizeof(t_block)`).
- `find_zone_containing_block` validates the pointer by walking every zone's block list and
  matching the exact header address — a pointer we never handed out is ignored rather than
  corrupting state.
- TINY/SMALL frees just flip the flag and coalesce; the pool stays mapped for reuse.
- A fully-freed **LARGE** zone is **unlinked and `munmap`'d** immediately, returning its
  pages to the OS.

> Validation here is by list membership (O(total blocks)), which is robust but not a
> fast-path. See [§7](#7-design-trade-offs--known-limitations).

### 5.6 `merge_adjacent_free_blocks` (coalescing)

```c
void merge_adjacent_free_blocks(t_block *block) {
    if (block->next && block->next->free) {                 // merge with the block after
        block->size += sizeof(t_block) + block->next->size; // reclaim its header too
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    if (block->prev && block->prev->free) {                 // merge with the block before
        block->prev->size += sizeof(t_block) + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}
```

When a block is freed, if the neighbor on either side is also free, fuse them into a single
larger free block — and reclaim the now-redundant header(s) as usable space (`+ sizeof(t_block)`).
Because the list is address-ordered and doubly-linked, both directions are O(1). This is
the primary defense against external fragmentation.

### 5.7 `realloc`

[`src/realloc.c`](../src/realloc.c)

```c
void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);     // realloc(NULL, n) == malloc(n)
    if (size == 0) { free(ptr); return NULL; }  // realloc(ptr, 0) == free + NULL

    pthread_mutex_lock(&g_mutex);
    t_block *block = (t_block*)((char*)ptr - sizeof(t_block));
    t_zone  *zone  = find_zone_containing_block(block);
    if (!zone) { pthread_mutex_unlock(&g_mutex); return NULL; }

    if (block->size >= size) {                 // shrinking / fits in place
        split_block(block, size);              // trim and free the tail
        pthread_mutex_unlock(&g_mutex);
        return ptr;                            // same pointer, no copy
    } else {                                   // growing beyond the block
        pthread_mutex_unlock(&g_mutex);        // unlock before re-entrant malloc/free
        void *new_ptr = malloc(size);          // allocate fresh
        if (!new_ptr) return NULL;             // (old block is left intact on failure)
        ft_memcpy(new_ptr, ptr, block->size);  // copy the old contents over
        free(ptr);                             // release the old block
        return new_ptr;
    }
}
```

Two cases:

- **Shrink / same size** → keep the pointer, `split_block` releases any now-excess tail. No
  data movement.
- **Grow** → allocate a new block, copy the old bytes (`block->size` of them), free the old.
  The unlock-before-`malloc` is deliberate: `malloc` and `free` lock the mutex themselves,
  so holding it here would deadlock with a non-recursive mutex.

### 5.8 `show_alloc_mem`

[`src/show_alloc_mem.c`](../src/show_alloc_mem.c)

A debug dump (not part of libc). It locks the mutex, walks every zone, prints the zone type
and base address, then for each **used** block prints its `start - end : size bytes` range,
and finally a grand total. Output looks like:

```
TINY : 0x7f3c1a000000
0x7f3c1a000020 - 0x7f3c1a000040 : 32 bytes
0x7f3c1a000050 - 0x7f3c1a0000a0 : 80 bytes
SMALL : 0x7f3c19800000
0x7f3c19800020 - 0x7f3c19800220 : 512 bytes
LARGE : 0x7f3c19000000
0x7f3c19000020 - 0x7f3c190f4260 : 1000000 bytes
Total : 1000624 bytes
```

It deliberately uses only `write(2)` and hand-rolled number/hex formatting — calling
`printf` would risk re-entering `malloc` from inside the allocator.

---

## 6. Building, using, testing

**Prerequisites:** GCC, GNU Make, a Linux/Unix host, pthreads. (Valgrind for the leak/race
targets.)

### Build

```bash
make            # builds libft, then libft_malloc_$(HOSTTYPE).so + libft_malloc.so symlink
make clean      # remove object files
make fclean     # remove objects + the .so and symlink
make re         # fclean + all
```

The output is a **shared object**. `HOSTTYPE` (e.g. `x86_64_Linux`) is baked into the name
so different architectures don't clash; `libft_malloc.so` is a symlink to the current one.
Compiled with `-Wall -Wextra -Werror -fPIC -g` and linked `-shared -lpthread`.

### Use it as a drop-in allocator (`LD_PRELOAD`)

`LD_PRELOAD` tells the dynamic linker (`ld.so`) to load *our* library before libc, so our
`malloc`/`free`/`realloc` symbols **override** libc's for any program — no recompilation:

```bash
export LD_LIBRARY_PATH=.
export LD_PRELOAD=$(pwd)/libft_malloc.so
./your_program            # now every malloc call goes through ft_malloc
```

Unset `LD_PRELOAD` to go back to the system allocator.

### Test

```bash
make test       # build test/test.base.c against the lib and run it
make vtest      # same under Valgrind (--leak-check=full --track-origins=yes)
make htest      # same under Helgrind (--tool=helgrind) for data races / lock order
```

Notes baked into the Makefile: Valgrind/Helgrind instrumentation perturbs the
`show_alloc_mem` output (focus on the leak/race findings, not the addresses), and the
double-free test is expected to surface exactly one Valgrind error.

---

## 7. Design trade-offs & known limitations

| Area | Current choice | Trade-off / possible improvement |
|------|----------------|----------------------------------|
| **Locking** | One global mutex around every call | Correct and simple, but serializes all threads. Per-zone or per-size-class locks, or lock-free free-lists, would scale better. |
| **Fit policy** | First-fit | Fast; more fragmentation than best-fit or segregated free-lists. |
| **Free validation** | Linear scan of all blocks (`find_zone_containing_block`) | Robust against bad pointers, but O(n). A real allocator derives the zone from the address (e.g. align-down to the mapping) in O(1). |
| **`malloc(0)`** | Returns `NULL` | libc typically returns a unique, freeable pointer. Some programs treat `NULL` as failure. |
| **TINY/SMALL reclaim** | Pools stay mapped for the process lifetime | Only LARGE zones are `munmap`'d. Fully-empty TINY/SMALL pools could also be unmapped to lower RSS. |
| **Double free** | A second `free` finds the block still valid and may re-coalesce | No poisoning/quarantine; detectable under Valgrind but not hardened against. |
| **`realloc` grow** | Always allocate-copy-free | Could try to coalesce with the adjacent free block to grow in place and skip the copy. |

These are intentional simplifications appropriate for the project's scope; the table doubles
as a roadmap.

---

## 8. Glossary

- **Allocator** — code that hands out and reclaims heap memory (`malloc`/`free`).
- **Block** — one sub-division of a zone: a live allocation or a free hole, prefixed by a
  `t_block` header.
- **Coalescing / merging** — fusing adjacent free blocks into one larger free block.
- **External fragmentation** — free memory exists but is split into chunks too small to use.
- **First-fit** — pick the first free block large enough.
- **Header (intrusive metadata)** — bookkeeping stored immediately before the user's bytes.
- **Internal fragmentation** — wasted space inside an allocated block (e.g. alignment pad).
- **`LD_PRELOAD`** — env var making `ld.so` load a library first, overriding libc symbols.
- **`mmap` / `munmap`** — syscalls to map / unmap memory regions from the kernel.
- **Page** — the kernel's unit of memory (4096 bytes on Linux).
- **Splitting** — carving a smaller block out of a larger free block.
- **Zone** — one `mmap`'d region; a TINY/SMALL pool or a single LARGE allocation.

## 9. References

- `man 2 mmap`, `man 2 munmap`, `man 3 malloc`, `man 8 ld.so`, `man 3 pthread_mutex_lock`
- [glibc malloc internals](https://sourceware.org/glibc/wiki/MallocInternals)
- [TCMalloc design](https://google.github.io/tcmalloc/design.html) — size-class inspiration
- Project design notes: [`docs/notes/`](notes/) —
  [zones & blocks](notes/zones_and_blocks.md),
  [alignment](notes/memory_alignment.md),
  [thread safety](notes/thread_safety.md),
  [overview](notes/libft_malloc.md)

---

*42 School — RNCP7 · Unix/Kernel specialization*
