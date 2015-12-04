#ifndef _H_PdfStrings
#define _H_PdfStrings
#pragma once

#include "PdfAlloc.h"

extern char *pd_strdup(t_pdmempool *alloc, const char *str);

// Copy string from src to dst, writing not more than destlen chars at dst.
// If dst is NULL or destlen is 0, has absolutely no effect.
// Otherwise if src is NULL it is treated as if it were ""
// Otherwise, up to destlen-1 or strlen(src), whichever is less, characters are copied
// from src to dst. A trailing 0 is appended.
// So: if dst != NULL and destlen > 0, a valid 0-terminated C string
// is always stored in dst.
extern void pd_strcpy(char *dst, size_t destlen, const char *src);

extern pdint32 pd_strcmp(const char *s1, const char *s2);

#endif
