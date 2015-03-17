#ifndef _H_PdfHash
#define _H_PdfHash
#pragma once

#include "PdfElements.h"
#include "PdfAlloc.h"


/* moved to PdfElements.h */
/* typedef struct t_pdhashatomtovalue t_pdhashatomtovalue; */

extern t_pdhashatomtovalue *pd_hashatomtovalue_new(t_pdallocsys *alloc, pduint32 initialsize);
extern void pd_hashatomtovalue_free(t_pdhashatomtovalue *table);
extern void pd_hashatomtovalue_put(t_pdhashatomtovalue *table, t_pdatom key, t_pdvalue value);
extern t_pdvalue pd_hashatomtovalue_get(t_pdhashatomtovalue *table, t_pdatom key, pdbool *success);
extern int pd_hashatomtovalue_contains(t_pdhashatomtovalue *table, t_pdatom key);
extern t_pdallocsys *pd_hashatomtovalue_getallocsys(t_pdhashatomtovalue *table);

typedef pdbool(*f_pdhashatomtovalue_iterator)(t_pdatom atom, t_pdvalue value, void *cookie);

extern void pd_hashatomtovalue_foreach(t_pdhashatomtovalue *table, f_pdhashatomtovalue_iterator iter, void *cookie);

#endif