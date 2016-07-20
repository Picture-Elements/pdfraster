#ifndef _H_PdfOS
#define _H_PdfOS
#pragma once

#include "PdfPlatform.h"

// Signature of the memory allocation (malloc) function to be used by the Pdf library
typedef void *(*fAllocate)(size_t bytes);
// (The signature of) the matching free.
typedef void (*fFree)(void *ptr);

typedef enum {
	REPORTING_INFO,			// useful to know but not bad news.
	REPORTING_WARNING,		// a potential problem - but execution can continue.
	REPORTING_PDF_RASTER,	// a violation of the PDF/raster specification was detected.
	REPORTING_API,			// an invalid request was made to this API.
	REPORTING_MEMORY,		// memory allocation failed.
	REPORTING_IO,			// PDF read or write failed unexpectedly.
	REPORTING_LIMIT,		// a built-in limitation of this library was exceeded.
	REPORTING_INTERNAL,		// an 'impossible' internal state has been detected.
	REPORTING_FATAL			// none of the above, and the current API call cannot complete.
} ReportingLevel;

// Signature of the error reporting function, currently not (much?) used.
// The library calls this function to report errors, warnings and obscure information.
// Parameters:
// msg will be a 0-terminated constant string in English with no line-breaks, which
// is only valid & constant *during the call* to the error reporting function.
// In other words, if you want to use it after the call, you must make a copy.
// err is a precise/unique library-defined error code, generated at ONE spot in the library.
// level provides some hint of the kind and severity of the error.
typedef void (*fReportError)(const char* msg, int err, ReportingLevel level);

// Signature of the output function: Writes length bytes from data+offset, to some destination dest.
// Note: As it might change for each document written, the value of dest is provided separately.
// Returns the number of bytes written.
// A return value < length indicates an error such as device full.
typedef int (*fOutputWriter)(const pduint8 *data, pduint32 offset, pduint32 length, void *dest);

// Signature of a memory-filling function (typically, memset)
typedef void (*fMemSet)(void *ptr, pduint8 value, size_t count);

typedef struct {
	fAllocate			alloc;
	fFree				free;
	fReportError		reportError;
	fOutputWriter		writeout;
	void*				writeoutcookie;
	fMemSet				memset;
	struct t_pdmempool *allocsys;
} t_OS;

// Some useful, primitive functions provided both for the use
// of the library and for clients in case they are not readily available
// in the execution environment

extern pdint32 pdstrlen(const char *s);

// convert a signed integer to NUL-terminated string at dst.
// Can write up to 12 chars (counting the trailing NUL) at dst.
extern char *pditoa(pdint32 i, char* dst);

extern char *pdftoa(pddouble n);
extern char * pdftoaprecision(pddouble n, pddouble precision);

// use this macro to suppress "unreferenced formal parameter" warnings
#define UNUSED_FORMAL(x) ((void)(x))

#endif
