#ifndef _H_PdfString
#define _H_PdfString
#pragma once

#include "PdfAlloc.h"
#include "PdfElements.h"

extern t_pdstring *pd_string_new(t_pdallocsys *alloc, char *string, pduint32 len, pdbool isbinary);
extern void pd_string_free(t_pdstring *str);
extern pduint32 pd_string_length(t_pdstring *str);
extern void pd_string_set(t_pdstring *str, char *string, pduint32 len, pdbool isbinary);
extern pdbool pd_string_is_binary(t_pdstring *str);
extern pduint8 pdstring_char_at(t_pdstring *str, pduint32 index);

typedef pdbool(*f_pdstring_foreach)(pduint32 index, pduint8 c, void *cookie);
extern void pd_string_foreach(t_pdstring *str, f_pdstring_foreach iter, void *cookie);

#endif