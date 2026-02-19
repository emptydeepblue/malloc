# ft_malloc - Custom Dynamic Memory Allocator

A 42 School project implementing `malloc`, `free`, and `realloc` using `mmap`/`munmap` with optimized TINY/SMALL/LARGE zones.

## Features
- ✅ Thread-safe with POSIX mutexes
- ✅ 16-byte memory alignment
- ✅ Zone-based allocation for performance
- ✅ No libc malloc dependency
- ✅ Comprehensive testing

## Build & Test
```bash
make          # Build library
make test     # Run tests
make vtest    # Valgrind memory check
make htest    # Helgrind thread check
```

## Usage
```bash
export LD_PRELOAD=./libft_malloc.so
./your_program
```

## Project Structure
```
├── inc/ft_malloc.h    # Header
├── src/
│   ├── malloc.c      # Core allocation + globals
│   ├── free.c        # Deallocation
│   ├── realloc.c     # Reallocation
│   └── show_alloc_mem.c # Memory display
├── libft/            # 42 libft
├── docs/             # Documentation
├── test.c            # Tests
└── Makefile          # Build config
```

## Zones
- **TINY**: ≤64 bytes, 4MB zones
- **SMALL**: 65-1024 bytes, 8MB zones
- **LARGE**: >1024 bytes, direct mmap

## Documentation
See `docs/` for technical details.

---

*42 School Project - Dynamic Memory Allocation*
