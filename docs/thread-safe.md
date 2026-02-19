
# Thread-Safety in malloc Implementation
The mutex (`pthread_mutex_t g_mutex`) is used in the malloc implementation to ensure thread-safety, as required by the project subject ("one for the thread-safe"). Here's why it's necessary and how it works:

## Why Use a Mutex?
**Shared Global State**: the allocator uses global variables (`g_zones` linked list) to track memory zones and blocks. Multiple threads calling `malloc`, `free`, or `realloc` simultaneously could access/modify this shared data concurrently, leading to race conditions (e.g., corrupted pointers, double-free errors, or memory leaks).
**Thread-Safety Requirement**: The subject allows `libpthread` functions, and standard `malloc` must be thread-safe. Without locking, the implementation isn't safe for multi-threaded programs.
**Prevention of Data Races**: The mutex serializes access—only one thread can execute critical sections (e.g., searching zones, allocating blocks) at a time, ensuring atomicity.

## How It's Used in the Code
**Lock Before Operations**: `pthread_mutex_lock(&g_mutex);` at the start of `malloc` (and similarly in `free`, `realloc`, `show_alloc_mem`).
**Unlock After**: `pthread_mutex_unlock(&g_mutex);` at the end, releasing the lock.
**Initialization**: Statically initialized in `src/init.c` with `PTHREAD_MUTEX_INITIALIZER` (no runtime init needed).
**Scope**: Locks the entire function to keep it simple, though finer-grained locking (e.g., per zone) could optimize performance in high-concurrency scenarios.

## Benefits and Trade-offs
**Pros**: Prevents crashes and undefined behavior in multi-threaded apps (e.g., using `malloc` in threads).
**Cons**: Slight performance overhead (blocking other threads), but acceptable for correctness. If not used, tests or real programs might fail unpredictably.

If you remove the mutex, the code compiles but isn't thread-safe—test with multiple threads to see issues. For single-threaded use, it's still fine (locks are no-ops if uncontended). If you have more questions (e.g., about implementation details), ask!