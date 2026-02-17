# malloc

A custom implementation of dynamic memory allocation functions.

## Description

This project implements `malloc()`, `free()`, and `realloc()` using `mmap()` and `munmap()` system calls, with memory zones optimized for different allocation sizes (TINY, SMALL, LARGE).

## Build

```bash
make
```

## Usage

```bash
export LD_PRELOAD=./libft_malloc.so
./your_program
```

## Functions

- `void *malloc(size_t size)` - Allocate memory
- `void free(void *ptr)` - Free allocated memory
- `void *realloc(void *ptr, size_t size)` - Reallocate memory
- `void show_alloc_mem()` - Display memory allocations

## Project Structure

```
.
├── inc/       # Header files
├── src/           # Source files
├── libft/         # libft library
├── Makefile       # Build configuration
└── README.md      # This file
```
