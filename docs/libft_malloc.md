# ft_malloc - Custom Malloc Implementation
This document provides an overview of the `ft_malloc` project, a custom implementation of the `malloc`, `free`, and `realloc` functions in C. The project is designed to be thread-safe, efficient, and compliant with standard memory allocation requirements.

## Architecture and Data Structures
- **Global Variables**:
  - `t_zone *g_zones`: Linked list of memory zones.
  - `pthread_mutex_t g_mutex`: Mutex for thread-safety.
- **Structs**:
  - `t_block`: Represents a memory block (size, free flag, next and previous block pointers).
  - `t_zone`: Represents a zone (type, total size, start address, blocks list, next zone).
- **Constants**:
  - `ALIGNMENT = 16`
  - `TINY_MAX = 64`
  - `SMALL_MAX = 512`
  - `TINY_ZONE_SIZE = 4MB`
  - `SMALL_ZONE_SIZE = 8MB` (page-aligned)

## Malloc Logic
`malloc(size)` allocates memory:
1. **Lock**: Lock `g_mutex` to ensure thread-safety.
2. **Handle Zero Size**: If `size` is 0, return NULL (aligned but not useful).
3. **Determine Zone Type**: Based on `size`, determine if it's TINY, SMALL, or LARGE.
4. **Find/Create Zone**: Search `g_zones` for a suitable zone; if none, create a new one with `mmap`. For LARGE, always create a new zone.  
5. **Find Block**: Traverse blocks in the zone to find a free block ≥ requested size.
6. **Split Block**: If the block is larger than needed + overhead, split it into an allocated block and a free block.
7. **Mark Allocated**: Set the block's free flag to 0.
8. **Return Pointer**: Return a pointer to the user data (after the block header).
9. **Unlock**: Release the mutex.

## Free Logic
`free(ptr)` deallocates memory:
1. **Lock**: Lock `g_mutex`.
2. **Find Block**: Traverse zones/blocks to find the block corresponding to `ptr`.
3. **Mark Free**: Set block's free flag to 1.
4. **Merge**: Merge with adjacent free blocks (merge_blocks).
5. **Unmap LARGE**: If it's a LARGE zone and fully free, `munmap` the entire zone and remove from list.
6. **Unlock**: Release mutex. 

## Realloc Logic
`realloc(ptr, size)` resizes an allocation:
1. **Lock**: Lock `g_mutex`.
2. **Handle NULL**: If `ptr` is NULL, call `malloc(size)`.
3. **Handle Zero Size**: If `size` is 0, call `free(ptr)` and return NULL.
4. **Find Block**: Locate the block for `ptr`.
5. **Check Size**: If new size <= current block size, optionally split block.
6. **Allocate New**: If new size > current block, allocate new block and copy data.
7. **Free Old**: Free the old block.
8. **Unlock**: Release mutex.

## show_alloc_mem Logic
`show_alloc_mem()` displays allocated memory:
1. **Lock**: Lock `g_mutex` to ensure thread-safety during traversal.
2. **Loop**: Traverse `g_zones` linked list.
3. **Print Zone**: For each zone, print type, start address, and size.
4. **Print Blocks**: For each block in the zone, if allocated, print start address and size.
5. **Unlock**: Release the mutex after traversal.