# Zones and Blocks in ft_malloc

## Overview
In this custom `malloc` implementation, memory is managed using **zones** and **blocks** to efficiently handle allocations of different sizes. This design minimizes system calls (`mmap`/`munmap`) while ensuring fast allocation and deallocation.

## What are Zones?
Zones are large, pre-allocated memory regions obtained via `mmap(2)`. They group allocations by size to reduce overhead:
- **TINY Zone**: For small allocations (1-64 bytes). Size: 4MB (page-aligned), holds ~100+ allocations.
- **SMALL Zone**: For medium allocations (65-512 bytes). Size: 8MB, holds ~100+ allocations.
- **LARGE Zone**: For big allocations (>512 bytes). Each is a separate zone (direct `mmap`), sized exactly to the allocation + overhead.

### Why Zones?
- **Performance**: Reusing zones avoids frequent `mmap` calls (expensive). TINY/SMALL zones are "pre-allocated" pools.
- **Efficiency**: Zones are multiples of `getpagesize()` (Linux: 4096 bytes).
- **Organization**: Zones are linked in a global list (`g_zones`), allowing traversal for `free` and `show_alloc_mem`.

### Zone Structure
Each `t_zone` struct contains:
- `type`: TINY, SMALL, or LARGE.
- `size`: Total zone size (in bytes).
- `start`: Starting address (from `mmap`).
- `blocks`: Pointer to the first block in the zone.
- `next`: Pointer to the next zone.

## What are Blocks?
Blocks are subdivisions within a zone, representing individual allocations or free space. Each `t_block` struct tracks:
- `size`: Block size (in bytes, aligned).
- `free`: Flag (1 = free, 0 = allocated).
- `next`: Pointer to the next block in the zone.

### Why Blocks?
- **Granularity**: Zones are large; blocks allow fine-grained allocation (e.g., split a 4MB zone into many 64-byte blocks).
- **Reuse**: Free blocks can be reused for new allocations (first-fit strategy).
- **Merging**: Adjacent free blocks are merged to reduce fragmentation.

### Block Management
- **Allocation**: Search for a free block ≥ requested size; split if larger.
- **Deallocation**: Mark as free; merge with neighbors.
- **Splitting**: In `split_block()`, if a block is too big, create a new free block after the allocated portion.

## How Zones and Blocks Work Together
1. **Allocation**:
   - Determine zone type based on size.
   - Find/create zone.
   - Find/split block within zone.
   - Return pointer to user data (after block header).

2. **Deallocation**:
   - Find the block via pointer arithmetic.
   - Mark free and merge.
   - For LARGE zones, `munmap` if fully free.

3. **Visualization**:
   - `show_alloc_mem()` traverses zones/blocks, printing allocated ranges.

### Example
- `malloc(32)` (TINY): Creates/finds TINY zone, allocates a 32-byte block (plus header), returns aligned pointer.
- `malloc(1000)` (LARGE): Creates new LARGE zone with one 1000-byte block.

### Constants
- `TINY_MAX = 64`, `SMALL_MAX = 512`
- `TINY_ZONE_SIZE = 4 * 1024 * 1024`, `SMALL_ZONE_SIZE = 8 * 1024 * 1024`