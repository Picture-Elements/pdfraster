#ifndef _H_PdfAlloc
#define _H_PdfAlloc
#pragma once

#include "PdfOS.h"

typedef struct t_pdallocsys t_pdallocsys;

// Create and return an allocation pool, which sub-manages
// memory from the underlying 'os' platform.
extern struct t_pdallocsys *pd_alloc_sys_new(t_OS *os);

// Return a memory block to a pool.
// ptr = a pointer previously returned by __pd_alloc and not since freed.
// Or NULL, which is ignored.
extern void pd_free(void *ptr);

// Return the number of blocks currently allocated from a pool
extern size_t pd_get_block_count(t_pdallocsys* pool);

// Return the total bytes currently allocated to blocks a pool.
// Does not include any overhead, just the sum of the sizes alloc'd and not yet freed.
extern size_t pd_get_bytes_in_use(t_pdallocsys* pool);

extern size_t pd_get_block_size(void* block);

// Frees all the blocks in a pool.
// Same effect as calling pd_free on every outstanding block in the pool.
extern void pd_pool_clean(t_pdallocsys* pool);

// (internal) Allocate a block of memory in/from an allocation pool.
// allocsys = allocation pool to allocate from/in.
// nb = number of bytes (at least) to allocate
// loc = string for tracing/debugging allocations, or NULL.
// returns: pointer to at least nb bytes of memory not in use for anything else,
// associated with the allocation pool and filled with 0's.
extern void *__pd_alloc(t_pdallocsys *allocsys, size_t nb, char *allocatedBy);

// Return the memory pool that a block was allocated from.
// If ptr is NULL, returns NULL.
extern t_pdallocsys *__pd_get_pool(void *ptr);

// (internal) Return a memory block to a pool.
// ptr = a pointer previously returned by __pd_alloc and not since freed.
// Or NULL, which is ignored.
// verifyptr = if truthy and DEBUG build, do extra validation
extern void __pd_free(void *ptr, pdbool veryifyptr);

// Release an allocation pool.
// This releases all memory associated with the pool back to the
// platform underlying the pool.
extern void pd_alloc_sys_free(t_pdallocsys *sys);

#define ELEMENTS(a) (sizeof(a)/sizeof((a)[0]))

// do a pd_alloc of bytes from same pool as an existing block
#define pd_alloc_same_pool(block, bytes) pd_alloc(__pd_get_pool(block), bytes)

#if PDDEBUG
#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ " : " S2(__LINE__)
#define pd_alloc(allocsys, bytes) __pd_alloc(allocsys, bytes, LOCATION)
#else
#define pd_alloc(allocsys, bytes) (__pd_alloc(allocsys, bytes, 0))
#endif
#endif

