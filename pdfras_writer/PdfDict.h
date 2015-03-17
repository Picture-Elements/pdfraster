#ifndef _H_PdfDict
#define _H_PdfDict
#pragma once

#include "PdfAlloc.h"
#include "PdfElements.h"
#include "PdfHash.h"
#include "PdfStreaming.h"
#include "PdfDatasink.h"

extern t_pdvalue dict_new(t_pdallocsys *allocsys, pdint32 initialsize);
extern void dict_free(t_pdvalue dict);
extern t_pdvalue dict_get(t_pdvalue dict, t_pdatom key, pdbool *success);
extern t_pdvalue dict_put(t_pdvalue dict, t_pdatom key, t_pdvalue value);
extern pdbool dict_is_stream(t_pdvalue dict);

extern void dict_foreach(t_pdvalue dict, f_pdhashatomtovalue_iterator iter, void *cookie);

extern void dict_write(t_pdoutstream *os, t_pdvalue dict);


typedef void(*f_on_datasink_ready)(t_datasink *sink, void *eventcookie);

extern t_pdvalue stream_new(t_pdallocsys *allocsys, t_pdxref *xref, pdint32 initialsize, f_on_datasink_ready ready, void *eventcookie);
extern void stream_free(t_pdvalue stream);
extern void stream_set_on_datasink_ready(t_pdvalue stream, f_on_datasink_ready ready, void *eventcookie);
extern void stream_write_contents(t_pdvalue stream, t_pdoutstream *outstm);

#endif