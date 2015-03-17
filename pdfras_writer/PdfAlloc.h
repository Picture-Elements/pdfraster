#ifndef _H_PdfAlloc
#define _H_PdfAlloc
#pragma once

#include "PdfOS.h"

typedef struct t_pdallocsys t_pdallocsys;

extern struct t_pdallocsys *pd_alloc_sys_new(t_OS *os);
extern void *__pd_alloc(t_pdallocsys *allocsys, size_t bytes, char *allocatedBy);
extern void __pd_free(t_pdallocsys *allocsys, void *ptr, pdbool veryifyptr);
extern void pd_alloc_sys_free(t_pdallocsys *sys);

#if PDDEBUG
#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ " : " S2(__LINE__)
#define pd_alloc(allocsys, bytes) __pd_alloc(allocsys, bytes, LOCATION)
#define pd_free(allocsys, ptr) (__pd_free(allocsys, ptr, 1))
#else
#define pd_alloc(allocsys, bytes) (__pd_alloc(allocsys, bytes, 0))
#define pd_free(allocsys, ptr) (__pd_free(allocsys, ptr, 0))
#endif
#endif

