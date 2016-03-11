#ifndef _H_PdfStandardObjects
#define _H_PdfStandardObjects
#pragma once

#include "PdfAlloc.h"
#include "PdfXrefTable.h"
#include "PdfValues.h"
#include "PdfDict.h"
#include "PdfContentsGenerator.h"

extern t_pdvalue pd_catalog_new(t_pdmempool *alloc, t_pdxref *xref);
extern t_pdvalue pd_info_new(t_pdmempool *alloc, t_pdxref *xref);
extern t_pdvalue pd_trailer_new(t_pdmempool *alloc, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info);
t_pdvalue pd_generate_file_id(t_pdmempool *alloc, t_pdvalue info);

extern t_pdvalue pd_page_new_simple(t_pdmempool *alloc, t_pdxref *xref, t_pdvalue catalog, double width, double height);

// Append a page to the page catalog
extern void pd_catalog_add_page(t_pdvalue catalog, t_pdvalue page);

extern t_pdvalue pd_contents_new(t_pdmempool *alloc, t_pdxref *xref, t_pdcontents_gen *gen);
extern void pd_page_add_image(t_pdvalue page, t_pdatom imageatom, t_pdvalue image);

// date/time strings

// format a time_t value as an (ASCII) string in PDF time format
// and return the updated write pointer i.e. pointer to the trailing NUL.
// Returns NULL if dstlen < 23
char* pd_format_time(time_t t, char* dst, size_t dstlen);

// format a time_t value as a string in XMP time format (max 25 chars)
// and return the updated write pointer i.e. pointer to the trailing NUL.
// Returns NULL if dstlen < 26
char* pd_format_xmp_time(time_t t, char* dst, size_t dstlen);

// construct a string object containing 'now' as a PDF time string
t_pdvalue pd_make_now_string(t_pdmempool *alloc);

// construct a string object containing a time_t as a PDF time string
t_pdvalue pd_make_time_string(t_pdmempool *alloc, time_t t);

// Create a new Metadata stream
t_pdvalue pd_metadata_new(t_pdmempool *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie);

#endif
