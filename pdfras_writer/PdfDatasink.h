#ifndef _H_PdfDatasink
#define _H_PdfDatasink
#pragma once

#include "PdfValues.h"
#include "PdfAlloc.h"

typedef struct t_datasink t_datasink;

typedef void (*f_sink_begin)(void *cookie);
typedef pdbool(*f_sink_put)(const pduint8 *buffer, pduint32 offset, pduint32 len, void *cookie);
typedef void (*f_sink_end)(void *cookie);
typedef void(*f_sink_free)(void *cookie);

extern t_datasink *pd_datasink_new(t_pdmempool *alloc, f_sink_begin begin, f_sink_put put, f_sink_end end, f_sink_free free, void *cookie);
extern void pd_datasink_free(t_datasink *sink);
extern void pd_datasink_begin(t_datasink *sink);
extern pdbool pd_datasink_put(t_datasink *sink, const void* data, pduint32 offset, size_t len);
extern void pd_datasink_end(t_datasink *sink);

#endif
