# ft_malloc - Custom Malloc Implementation
This project implements a custom dynamic memory allocator `malloc` in C, adhering to the 42 School subject requirements. It mimics the behavior of standard `malloc` but uses `mmap(2)` and `munmap(2)` for memory management, with thread-safety via POSIX threads.

## Overview
- **Goal**: Provide a functional, thread-safe malloc library that handles small, medium, and large allocations efficiently.
- **Key Features**:
  - **Zones**: TINY (≤64 bytes), SMALL (65-512 bytes), LARGE (>512 bytes) to minimize mmap calls.
  - **Alignment**: All allocations aligned to 16 bytes for performance.
  - **Thread-Safety**: Uses a global mutex for all operations.
  - **No libc malloc**: Internal data structures use the library itself
- **Allocation Strategy**:
  - **Zone Creation**: `mmap` allocates page-aligned memory. Blocks start aligned to 16 bytes. For TINY/SMALL, one large free block initially; for LARGE, one allocated block.
  - **Splitting**: If a block is larger than needed + overhead, split into allocated + free remainder.

## Architecture and Data Structures
- **Global Variables (in init.c)**:
  - `t_zone *g_zones`: Linked list of memory zones.
  - `pthread_mutex_t g_mutex`: Mutex for thread-safety.
- **Structs (in ft_malloc.h)**:
  - `t_block`: Represents a memory block (size, free flag, next block pointer).
  - `t_zone`: Represents a zone (type, total size, start address, blocks list, next zone).
- **Constants**:
  - `ALIGNMENT = 16`
  - `TINY_MAX = 64`
  - `SMALL_MAX = 512`
  - `TINY_ZONE_SIZE = 4MB`
  - `SMALL_ZONE_SIZE = 8MB` (page-aligned)

## Malloc Logic
`malloc(size)` allocates memory in five steps:
1. **Lock and Align**  
- Lock `g_mutex`.
- Align size to 16 bytes using `(size + 15) & ~15`.
2. **Determine Zone Type**  
- TINY: size <= 64
- SMALL: size <= 512
- LARGE: size > 512
3. **Find or Create Zone**  
- For TINY/SMALL: Search existing zones for a free block (find_suitable_block).
- If none, create a new zone with mmap (create_zone).
- For LARGE: Always create a new zone (direct mmap for exact size + overhead).
4. **Allocate Block**  
- For TINY/SMALL: Find first-fit free block; split if too large (split_block).
- For LARGE: Use the single block in the zone.
- Mark block as allocated (free = 0).
- Return pointer: (char*)block + sizeof(t_block).
5. **Unlock**: Release mutex.
