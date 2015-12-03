#ifndef _H_PdfString
#define _H_PdfString
#pragma once

#include "PdfAlloc.h"
#include "PdfValues.h"

// construct a new string object from a constant char string.
extern t_pdstring *pd_string_new(t_pdallocsys *alloc, const char *string, pduint32 len, pdbool isbinary);

// free a string object
extern void pd_string_free(t_pdstring *str);

// return the length (in bytes) of a string object.
extern pduint32 pd_string_length(t_pdstring *str);

// return a read-only pointer to the data of a string object.
extern pduint8* pd_string_data(t_pdstring *str);

// return the byte at index in the data of a string object
extern pduint8 pdstring_char_at(t_pdstring *str, pduint32 index);

// replace the data of a string object with a character sequence of specified length
extern void pd_string_set(t_pdstring *str, const char *string, pduint32 len, pdbool isbinary);

extern pdbool pd_string_is_binary(t_pdstring *str);

extern pdbool pd_string_equal(t_pdstring *s1, t_pdstring *s2);

extern int pd_string_compare(t_pdstring *s1, t_pdstring *s2);

// call a function on each character in a string object
typedef pdbool(*f_pdstring_foreach)(pduint32 index, pduint8 c, void *cookie);
extern void pd_string_foreach(t_pdstring *str, f_pdstring_foreach iter, void *cookie);

#endif
