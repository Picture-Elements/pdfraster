#ifndef _H_PdfArray
#define _H_PdfArray
#pragma once

#include "PdfOS.h"
#include "PdfElements.h"
#include "PdfAlloc.h"

extern t_pdarray *pd_array_new(t_pdallocsys *alloc, pduint32 initialSize);
extern void pd_array_free(t_pdarray *arr);
extern pduint32 pd_array_size(t_pdarray *arr);
extern pduint32 pd_array_maxsize(t_pdarray *arr);
extern t_pdvalue pd_array_get(t_pdarray *arr, pduint32 index);
extern void pd_array_set(t_pdarray *arr, pduint32 index, t_pdvalue value);
extern void pd_array_insert(t_pdarray *arr, pduint32 index, t_pdvalue value);
extern void pd_array_add(t_pdarray *arr, t_pdvalue value);
extern t_pdvalue pd_array_remove(t_pdarray *arr, pduint32 index);

typedef pdbool(*f_pdarray_iterator)(t_pdarray *arr, pduint32 currindex, t_pdvalue value, void *cookie);
extern void pd_array_foreach(t_pdarray *arr, f_pdarray_iterator iter, void *cookie);

extern t_pdarray *pd_array_build(t_pdallocsys *alloc, pduint32 size, /* t_pdvalue value, */ ...);
extern t_pdarray *pd_array_buildints(t_pdallocsys *alloc, pduint32 size, /* pdint32 value, */ ...);
extern t_pdarray *pd_array_buildfloats(t_pdallocsys *alloc, pduint32 size, /* double value, */...);

#endif
