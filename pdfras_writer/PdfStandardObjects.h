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

// Format a PDF-format time string from a time_t.
void pd_get_time_string(time_t t, char szText[200]);

t_pdvalue pd_make_now_string(t_pdmempool *alloc);
t_pdvalue pd_make_time_string(t_pdmempool *alloc, time_t t);

// Create a new Metadata stream
t_pdvalue pd_metadata_new(t_pdmempool *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie);

#endif
