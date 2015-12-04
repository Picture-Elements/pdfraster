#ifndef _H_PdfHash
#define _H_PdfHash
#pragma once

#include "PdfValues.h"
#include "PdfAlloc.h"

/* moved to PdfValues.h */
/* typedef struct t_pdhashatomtovalue t_pdhashatomtovalue; */

// Allocate a new hashtable in pool, with room (initially) for initialCap entries.
extern t_pdhashatomtovalue *pd_hashatomtovalue_new(t_pdmempool *pool, pduint32 initialCap);

// Release a hashtable.
// Has no effect if table is NULL.
extern void pd_hashatomtovalue_free(t_pdhashatomtovalue *table);

// Get the number of (key,value) pairs in a hashtable.
// Returns 0 if table is NULL.
extern int pd_hashatomtovalue_count(t_pdhashatomtovalue *table);

// Associate key => value in hashtable - replace previous mapping for key if any.
// Has no effect if table is NULL or key is PDA_UNDEFINED_ATOM
extern void pd_hashatomtovalue_put(t_pdhashatomtovalue *table, t_pdatom key, t_pdvalue value);

// look up a key in a hashtable.
// returns the value associated with the key if found, and sets *success to true (PD_TRUE).
// returns pderrvalue() if key not found and sets *success to false (PD_FALSE).
extern t_pdvalue pd_hashatomtovalue_get(t_pdhashatomtovalue *table, t_pdatom key, pdbool *success);

// look up a key in a hashtable, return PD_TRUE if found, PD_FALSE if not.
extern pdbool pd_hashatomtovalue_contains(t_pdhashatomtovalue *table, t_pdatom key);

typedef pdbool(*f_pdhashatomtovalue_iterator)(t_pdatom atom, t_pdvalue value, void *cookie);

// Call iter(key, value, cookie) with each key,value pair in a hash table.
// Continue as long as iter returns non-zero, stop if iter returns 0.
// Has no effect if table OR iter are NULL.
extern void pd_hashatomtovalue_foreach(t_pdhashatomtovalue *table, f_pdhashatomtovalue_iterator iter, void *cookie);

// (Internal) Get the current capacity of a hashtable.
// Returns 0 if table is NULL.
extern int __pd_hashatomtovalue_capacity(t_pdhashatomtovalue *table);


#endif
