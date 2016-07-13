#ifndef _H_PdfDict
#define _H_PdfDict
#pragma once

#include "PdfAlloc.h"
#include "PdfValues.h"
#include "PdfHash.h"
#include "PdfStreaming.h"
#include "PdfDatasink.h"

extern t_pdvalue pd_dict_new(t_pdmempool *allocsys, pdint32 initialsize);
extern void pd_dict_free(t_pdvalue dict);
extern pdbool pd_dict_contains(t_pdvalue dict, t_pdatom key);
extern t_pdvalue pd_dict_get(t_pdvalue dict, t_pdatom key, pdbool *success);
extern t_pdvalue pd_dict_put(t_pdvalue dict, t_pdatom key, t_pdvalue value);
extern pdbool pd_dict_is_stream(t_pdvalue dict);

// Return the number of entries in a dictionary
extern int pd_dict_count(t_pdvalue dict);

extern void pd_dict_foreach(t_pdvalue dict, f_pdhashatomtovalue_iterator iter, void *cookie);

// * internal use only *
// Return the current capacity (no. of entries allocated) in a dictionary
extern int __pd_dict_capacity(t_pdvalue dict);
typedef void(*f_dict_pre_close_handler)(t_pdvalue dict, t_pdoutstream *os);
extern void __pd_dict_set_pre_close(t_pdvalue dict, f_dict_pre_close_handler handler);
extern void __pd_dict_pre_close(t_pdvalue dict, t_pdoutstream *os);

///////////////////////////////////////////////////////////////////////
// Streams - which are a species of Dictionary

// signature of a function that writes data to a sink.
typedef void(*f_on_datasink_ready)(t_datasink *sink, void *eventcookie);

// Create a new PDF Stream object and return an indirect reference to it.
// "All streams shall be indirect objects"
// pool is the allocation pool to use.
// xref is the XREF table to register the Stream in, to generate the reference.
// initialsize is the number of entries to preallocate in the Stream's dictionary.
// ready is a callback that can generate the contents of the Stream when requested.
// eventcookie is a cookie to pass to the ready function.
// <returns>A Reference to the created stream</returns>
extern t_pdvalue stream_new(t_pdmempool *pool, t_pdxref *xref, pdint32 initialsize, f_on_datasink_ready ready, void *eventcookie);

extern void stream_free(t_pdvalue stream);
extern void stream_set_on_datasink_ready(t_pdvalue stream, f_on_datasink_ready ready, void *eventcookie);
extern void stream_write_data(t_pdvalue stream, t_datasink* sink);

#endif
