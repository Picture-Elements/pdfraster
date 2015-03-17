#ifndef _H_PdfStrings
#define _H_PdfStrings
#pragma once

#include "PdfAlloc.h"

extern char *pd_strdup(t_pdallocsys *alloc, char *str);
extern char *pd_strcpy(char *dst, size_t destlen, char *src);
extern pdint32 pd_strcmp(char *s1, char *s2);

#endif