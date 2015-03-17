#ifndef _H_PdfOS
#define _H_PdfOS
#pragma once

#include "PdfPlatform.h"

typedef void *(*fAllocate)(size_t bytes);
typedef void (*fFree)(void *ptr);
typedef void (*fReportError)(/* TODO */);
typedef int (*fOutputWriter)(const pduint8 *data, pduint32 offset, pduint32 length, void *cookie);
typedef void (*fMemSet)(void *ptr, pduint8 value, size_t count);
typedef void (*fMemClear)(void *ptr, size_t count);

typedef struct {
	fAllocate alloc;
	fFree free;
	fReportError reportError;
	fOutputWriter writeout;
	void *writeoutcookie;
	fMemSet memset;
	fMemClear memclear;
	struct t_pdallocsys *allocsys;
} t_OS;

extern pdint32 pdstrlen(const char *s);
extern char *pditoa(pdint32 i);
extern char *pdftoa(pddouble n);
extern char * pdftoaprecision(pddouble n, pddouble precision);
extern char hexdigit(int c);

#endif