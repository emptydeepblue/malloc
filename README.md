# malloc

A custom dynamic memory allocator (`malloc`, `free`, `realloc`, `show_alloc_mem`) built directly on `mmap` / `munmap`, with TINY / SMALL / LARGE zones for performance and POSIX mutexes for thread safety.

**Specialization:** Unix/Kernel
**XP:** 9,450 · **People:** 1 · **Status:** ✅ Validated

---

## 🚀 Features

- Drop-in replacement for libc `malloc` / `free` / `realloc` via `LD_PRELOAD`
- Zone-based allocation: TINY, SMALL, and LARGE pools
- Thread-safe with POSIX mutexes
- 16-byte alignment, no libc malloc dependency
- `show_alloc_mem()` to dump live allocations
- Valgrind- and Helgrind-clean

## 🛠️ Build & Run

**Prerequisites:** GCC, GNU Make, Linux/Unix, pthread

```bash
make            # Build libft_malloc_$(HOSTTYPE).so + libft_malloc.so symlink
make clean      # Remove objects
make fclean     # Remove .so + objects
```

**Use it as a drop-in malloc:**
```bash
export LD_PRELOAD=$(pwd)/libft_malloc.so
./your_program
```

## 🧪 Testing

```bash
make test       # Run test suite
make vtest      # Valgrind (leak check)
make htest      # Helgrind (race / lock check)
```

## 📁 Project Structure

```
malloc/
├── src/
│   ├── malloc.c          # Core allocation + global state
│   ├── free.c            # Deallocation + zone reclamation
│   ├── realloc.c         # In-place / migrating realloc
│   └── show_alloc_mem.c  # Live-allocation dump
├── inc/ft_malloc.h       # Public header
├── libft/                # 42 libft (utility helpers)
├── test/                 # Test cases
├── docs/                 # Design notes
└── Makefile
```

## 🔧 Implementation

**Zones:**

| Zone   | Block size       | Backing                  |
|--------|------------------|--------------------------|
| TINY   | ≤ 64 bytes       | 4 MB pre-allocated zones |
| SMALL  | 65 – 1024 bytes  | 8 MB pre-allocated zones |
| LARGE  | > 1024 bytes     | Direct `mmap` per request|

**Technical details:**
- All memory acquired through `mmap` / released through `munmap` — no `brk`/`sbrk`
- Free blocks tracked per zone with intrusive metadata; coalesced on free
- Single global mutex serializes the allocator (simple, correct; future work: per-zone locks)
- `realloc` shrinks in place; grows in place if room, otherwise migrates and frees
- 16-byte alignment guaranteed for all returned pointers

## 📖 Full documentation

A complete, concept-first walkthrough (virtual memory, `mmap`, zones/blocks, alignment,
coalescing, thread-safety, `LD_PRELOAD`, and an annotated tour of every function) lives in
[docs/DOCUMENTATION.md](docs/DOCUMENTATION.md).

## 📚 Learning Objectives

- Memory allocator design: zones, free lists, coalescing
- Page-level memory management with `mmap` / `munmap`
- Thread safety with POSIX mutexes; reasoning about race conditions
- Shared library construction and `LD_PRELOAD` interception
- Helgrind / valgrind for verifying correctness

## 🔗 References

- `man 2 mmap`, `man 3 malloc`, `man 8 ld.so`
- [glibc malloc internals](https://sourceware.org/glibc/wiki/MallocInternals)
- [TCMalloc](https://google.github.io/tcmalloc/design.html) — for size-class inspiration

---

*42 School — RNCP7 · Unix/Kernel*
