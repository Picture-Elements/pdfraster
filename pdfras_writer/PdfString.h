#ifndef _H_PdfString
#define _H_PdfString
#pragma once

#include "PdfAlloc.h"
#include "PdfValues.h"

#ifdef __cplusplus
extern "C" {
#endif

// construct a new (non-binary) string object of length n.
// if string != NULL, initialize with 1st n chars of string.
extern t_pdstring *pd_string_new(t_pdmempool *alloc, pduint32 len, const char* string);

// construct a binary string of length n
// if data != NULL, initialize with 1st n bytes of data.
extern t_pdstring *pd_string_new_binary(t_pdmempool *pool, pduint32 len, const void* data);

// free a string object
extern void pd_string_free(t_pdstring *str);

// return the length (in bytes) of a string object.
extern pduint32 pd_string_length(t_pdstring *str);

// set the length in bytes of a string object to n.
// the contents of the string become (n bytes of) undefined.
pdbool pd_string_set_length(t_pdstring *str, pduint32 n);

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

#ifdef __cplusplus
}
#endif
#endif
