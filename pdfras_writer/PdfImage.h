#ifndef _H_PdfImage
#define _H_PdfImage
#pragma once

#include "PdfAlloc.h"
#include "PdfXrefTable.h"
#include "PdfValues.h"
#include "PdfDict.h"
#include "PdfContentsGenerator.h"

typedef enum {
	kCompNone,
	kCompFlate,
	kCompCCITT,
	kCompDCT,
	kCompJBIG2,
	kCompJPX
} e_ImageCompression;

typedef enum {
	kCCIITTG4,
	kCCITTG31D,
	kCCITTG32D
} e_CCITTKind;

// Create & return a calibrated grayscale colorspace
// with given BlackPoint, WhitePoint and Gamma - see PDF spec.
extern t_pdvalue pd_make_calgray_colorspace(t_pdmempool *alloc, double black[3], double white[3], double gamma);

// Create & return a standard sRGB colorspace (as an ICCBased colorspace).
// The ICC profile is per IEC61966-2.1
extern t_pdvalue pd_make_srgb_colorspace(t_pdmempool *alloc, t_pdxref *xref);

// Create & return an ICC Profile based 3-channel colorspace
// ...which is an array that references a stream containing an ICC color profile.
// Takes the address & size of the ICC profile data.
// The profile data must be kept alive & constant until the stream has been written.
extern t_pdvalue pd_make_iccbased_rgb_colorspace(t_pdmempool *alloc, t_pdxref *xref, const pduint8* prof_data, size_t prof_size);

extern t_pdvalue pd_image_new(t_pdmempool *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie,
	t_pdvalue width, t_pdvalue height, t_pdvalue bitspercomponent, e_ImageCompression comp, t_pdvalue compParms, t_pdvalue colorspace);

extern t_pdvalue pd_image_new_simple(t_pdmempool *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie,
	pduint32 width, pduint32 height, pduint32 bitspercomponent,
	e_ImageCompression comp, e_CCITTKind kind, pdbool ccittBlackIs1, t_pdvalue colorspace);

#endif
