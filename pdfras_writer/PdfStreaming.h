#ifndef _H_PdfStreaming
#define _H_PdfStreaming
#pragma once

#include "PdfAlloc.h"
#include "PdfElements.h"

typedef struct t_pdoutstream t_pdoutstream;
typedef struct t_pdxref t_pdxref;

extern t_pdoutstream *pd_outstream_new(t_pdallocsys *allocsys, t_OS *os);
extern void pd_outstream_free(t_pdoutstream *stm);

extern void pd_putc(t_pdoutstream *stm, char c);
extern void pd_puts(t_pdoutstream *stm, char *s);
extern void pd_putn(t_pdoutstream *stm, const pduint8 *s, pduint32 offset, pduint32 len);
extern void pd_putint(t_pdoutstream *stm, pdint32 i);
extern void pd_putfloat(t_pdoutstream *stm, pddouble f);

extern pduint32 pd_outstream_pos(t_pdoutstream *stm);

extern void pd_write_value(t_pdoutstream *stm, t_pdvalue value);
extern void pd_write_reference_declaration(t_pdoutstream *stm, t_pdvalue);
extern void pd_write_pdreference_declaration(t_pdoutstream *stm, t_pdreference *ref);

// Write the PDF header to the stream.
// version is a string with the header version e.g. "1.4"
// line2 is a string to be written after the first line of the header
// if line2 is NULL, a conventional 2nd line comment is written e.g. "%0xE2\xE3\xCF\xD3\n"
extern void pd_write_pdf_header(t_pdoutstream *stm, char *version, char *line2);

extern void pd_write_endofdocument(t_pdallocsys *alloc, t_pdoutstream *stm, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info);

#endif
