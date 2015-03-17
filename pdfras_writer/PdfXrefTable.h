#ifndef _H_PdfXrefTable
#define _H_PdfXrefTable

#include "PdfElements.h"
#include "PdfAlloc.h"
#include "PdfStreaming.h"

typedef struct t_pdxref t_pdxref;

extern pduint32 pd_reference_object_number(t_pdreference *ref);
extern pdbool pd_reference_is_written(t_pdreference *ref);
extern void pd_reference_mark_written(t_pdreference *ref);
extern t_pdvalue pd_reference_get_value(t_pdreference *ref);
extern void pd_reference_set_value(t_pdreference *ref, t_pdvalue value);
extern pduint32 pd_reference_get_position(t_pdreference *ref);
extern void pd_reference_set_position(t_pdreference *ref, pduint32 pos);

extern t_pdxref *pd_xref_new(t_pdallocsys *alloc);
extern void pd_xref_free(t_pdxref *xref);
extern t_pdvalue pd_xref_makereference(t_pdxref *xref, t_pdvalue value);
extern pdbool pd_xref_isreferenced(t_pdxref *xref, t_pdvalue value);
extern pdint32 pd_xref_size(t_pdxref *xref);

extern void pd_xref_writeallpendingreferences(t_pdxref *xref, t_pdoutstream *os);
extern void pd_xref_writetable(t_pdxref *xref, t_pdoutstream *os);
#endif
