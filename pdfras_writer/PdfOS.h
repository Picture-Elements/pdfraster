#ifndef _H_PdfOS
#define _H_PdfOS
#pragma once

#include "PdfPlatform.h"

// The signature of the memory allocation (malloc) function to be used by the Pdf library
typedef void *(*fAllocate)(size_t bytes);
// (The signature of) the matching free.
typedef void (*fFree)(void *ptr);

// (The signature of) the error reporting function, currently not (much?) used.
typedef void (*fReportError)(/* TODO */);

// (The signature of) the output function, that writes length bytes from data+offset, to 'output', using cookie
// Returns the number of bytes written.
// A return value < length indicates an error such as device full.
typedef int (*fOutputWriter)(const pduint8 *data, pduint32 offset, pduint32 length, void *cookie);

// (The signature of) a memory-filling function (typically, memset)
typedef void (*fMemSet)(void *ptr, pduint8 value, size_t count);

typedef struct {
	fAllocate alloc;
	fFree free;
	fReportError reportError;
	fOutputWriter writeout;
	void *writeoutcookie;
	fMemSet memset;
	struct t_pdmempool *allocsys;
} t_OS;

extern pdint32 pdstrlen(const char *s);
extern char *pditoa(pdint32 i);
extern char *pdftoa(pddouble n);
extern char * pdftoaprecision(pddouble n, pddouble precision);

// use this macro to suppress "unreferenced formal parameter" warnings
#define UNUSED_FORMAL(x) ((void)(x))

#endif
