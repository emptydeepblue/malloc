# Memory Alignment in ft_malloc

## What is Memory Alignment?
Memory alignment ensures that data is stored at addresses that are multiples of a specific byte boundary (e.g., 4, 8, or 16 bytes). This is crucial for performance and correctness on modern CPUs, which access memory in word-sized chunks (e.g., 64-bit systems use 8-byte words). Misaligned data can cause:
- **Performance penalties**: Extra CPU cycles for unaligned accesses.
- **Crashes or undefined behavior**: On some architectures, unaligned accesses are invalid.

In C, structs and allocations are often aligned to their largest member's size (e.g., `size_t` aligns to 8 bytes).

## Why Alignment Matters in Malloc
- **Hardware Requirements**: CPUs (e.g., x86-64) expect pointers and data to be aligned for efficient loads/stores. Unaligned allocations can lead to segfaults or slowdowns.
- **Standard Compliance**: `malloc(3)` must return pointers aligned to at least `alignof(max_align_t)` (usually 16 bytes on Linux), suitable for any type.
- **Project Requirement**: The subject states: "You must align the memory given by your malloc." This ensures compatibility with programs expecting aligned pointers.

## How Alignment is Implemented in ft_malloc
The code uses `#define ALIGNMENT 16` (16 bytes, a common safe value for modern systems).

### 1. **Aligning Allocation Sizes**
- Function: `align_size(size_t size)`
- Logic: Rounds up `size` to the next multiple of `ALIGNMENT` using bit manipulation:
  ```c
  size_t align_size(size_t size) {
      return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
  }
  ```
- Example: `align_size(1)` → 16, `align_size(10)` → 16, `align_size(17)` → 32.
- Why: Ensures allocated blocks are multiples of 16, so pointers remain aligned.

### 2. **Aligning Block Starts**
- In `create_zone()`: The `blocks` pointer is aligned to `ALIGNMENT`:
  ```c
  uintptr_t blocks_addr = (uintptr_t)(zone_start + sizeof(t_zone));
  blocks_addr = (blocks_addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
  new_zone->blocks = (t_block*)blocks_addr;
  ```
- Why: `zone_start` is page-aligned (from `mmap`), but `+ sizeof(t_zone)` may not be. This ensures the first block starts aligned, and subsequent allocations (via `split_block`) maintain alignment.

### 3. **Returned Pointers**
- `malloc` returns `(char*)block + sizeof(t_block)`, where `block` is aligned, and `sizeof(t_block)` is accounted for in alignment calculations.
- Result: All user pointers are 16-byte aligned.

### Testing Alignment
- Use `(uintptr_t)ptr % ALIGNMENT == 0` to check.
- In test.c, the alignment test verifies this.

### Edge Cases
- **Zero size**: `malloc(0)` returns `NULL` (aligned, but not useful).
- **Large sizes**: Still aligned, as zones are page-aligned.
- **Performance**: Alignment adds minimal overhead but prevents issues.
