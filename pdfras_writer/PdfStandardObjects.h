#ifndef _H_PdfStandardObjects
#define _H_PdfStandardObjects
#pragma once

#include "PdfAlloc.h"
#include "PdfXrefTable.h"
#include "PdfElements.h"
#include "PdfDict.h"
#include "PdfContentsGenerator.h"

extern t_pdvalue pd_catalog_new(t_pdallocsys *alloc, t_pdxref *xref);
extern t_pdvalue pd_info_new(t_pdallocsys *alloc, t_pdxref *xref, char *title, char *author, char *subject, char *keywords, char *creator, char *producer);
extern t_pdvalue pd_trailer_new(t_pdallocsys *alloc, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info);

typedef enum {
	kCompNone,
	kCompFlate,
	kCompCCITT,
	kCompDCT,
	kCompJBIG2,
	kCompJPX
} e_ImageCompression;

typedef enum {
	kDeviceGray,
	kDeviceRgb,
	kDeviceCmyk,
	kIndexed,
	kIccBased,
} e_ColorSpace;

typedef enum {
	kCCIITTG4,
	kCCITTG31D,
	kCCITTG32D
} e_CCITTKind;

extern t_pdvalue pd_image_new(t_pdallocsys *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie,
	t_pdvalue width, t_pdvalue height, t_pdvalue bitspercomponent, e_ImageCompression comp, t_pdvalue compParms, t_pdvalue colorspace);

extern t_pdvalue pd_image_new_simple(t_pdallocsys *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie,
	pduint32 width, pduint32 height, pduint32 bitspercomponent, e_ImageCompression comp, e_CCITTKind kind, pdbool ccittBlackIs1, e_ColorSpace colorspace);

extern t_pdvalue pd_page_new_simple(t_pdallocsys *alloc, t_pdxref *xref, t_pdvalue catalog, double width, double height);

// Append a page to the page catalog
extern void pd_catalog_add_page(t_pdvalue catalog, t_pdvalue page);

extern t_pdvalue pd_contents_new(t_pdallocsys *alloc, t_pdxref *xref, t_pdcontents_gen *gen);
extern void pd_page_add_image(t_pdvalue page, t_pdatom imageatom, t_pdvalue image);

// date/time strings
t_pdvalue pd_make_now_string(t_pdallocsys *alloc);
t_pdvalue pd_make_time_string(t_pdallocsys *alloc, time_t t);

#endif
