#ifndef _H_PdfStreaming
#define _H_PdfStreaming
#pragma once

// This module implements a logical stream for receiving a PDF file.
// A t_pdoutstream represents "where to write the PDF".
// It has the knowledge of how internal PDF objects and values are
// represented in the actual output.
// It has special hooks to handle encryption.

#include "PdfAlloc.h"
#include "PdfValues.h"

// When writing PDF, floats (reals) are only represented
// to this many digits of precision:
#define REAL_PRECISION 10

typedef struct t_pdoutstream t_pdoutstream;
typedef struct t_pdxref t_pdxref;
typedef struct t_pdencrypter t_pdencrypter;

extern t_pdoutstream *pd_outstream_new(t_pdallocsys *allocsys, t_OS *os);
extern void pd_outstream_free(t_pdoutstream *stm);

// Attach an 'encrypter' to this stream
extern void pd_outstream_set_encrypter(t_pdoutstream *stm, t_pdencrypter *crypt);

extern void pd_putc(t_pdoutstream *stm, char c);

// Write a 0-terminated C string to a stream.
// If stm is null, does absolutely nothing.
// s==NULL is treated as s==""
extern void pd_puts(t_pdoutstream *stm, char *s);

// Write a byte as two hex digits to a stream.
// Always writes 2 digits, uppercase.
// If stm is null, does absolutely nothing.
extern void pd_puthex(t_pdoutstream *stm, pduint8 b);

extern void pd_putn(t_pdoutstream *stm, const pduint8 *s, pduint32 offset, pduint32 len);

// Write a decimal representation of an integer to a stream.
// All possible values are handled: -2147483648 to 2147483647
// If i < 0, a '-' is prefixed.
// The output is written with the minimum number of characters needed.
extern void pd_putint(t_pdoutstream *stm, pdint32 i);

// Write a floating-point number to a stream.
// A traditional decimal fractional notation is used,
// [-]integer-part[.fraction]
// The leading minus is output if and only if the value f < 0.
// The fractional part is output if and if the value is not integral.
// Exact integral values are output as integers without the fractional part.
// This function is not obligated to output more than REAL_PRECISION significant digits.
// The decimal separator is *always* a period, '.'.
// Given any NaN, writes "nan".
// Given an infinity, writes "inf", with a leading minus-sign if negative.
extern void pd_putfloat(t_pdoutstream *stm, pddouble f);

// Return the current write offset (position) in the stream
extern pduint32 pd_outstream_pos(t_pdoutstream *stm);

/// Write a t_pdvalue to an output stream.
extern void pd_write_value(t_pdoutstream *stm, t_pdvalue value);

extern void pd_write_reference_declaration(t_pdoutstream *stm, t_pdvalue ref);

// Write the PDF header to the stream.
// version is a string with the header version e.g. "1.4".
// line2 is a string to be written after the first line of the header.
// if line2 is NULL, a conventional 2nd line comment is written e.g. "%0xE2\xE3\xCF\xD3\n"
// To suppress the 2nd line comment, use line2="" or line2="\n".
extern void pd_write_pdf_header(t_pdoutstream *stm, char *version, char *line2);

extern void pd_write_endofdocument(t_pdallocsys *alloc, t_pdoutstream *stm, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info);

#endif
