#ifndef _H_PdfRaster
#define _H_PdfRaster
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "PdfAlloc.h"
#include "PdfElements.h"
#include "PdfDatasink.h"

#define PDFRAS_API_LEVEL	1

#define PDFRAS_LIBRARY_VERSION "0.6"
// 0.6	spike	2015.02.13	bugfix! no EOL after the signature comment
// 0.5	spike	2015.01.13	prototype of 16-bit/channel image support
// 0.4	spike	2015.01.13	added physical page number & page-side
// 0.3	spike	2015.01.07	fixed bug in CCITTFaxDecode filter (mispelled!)
// 0.2	spike	2015.01.06	added pd_raster_set_rotation
// 0.1	spike	2014.12.14	1st version, using Steve Hawley's mini PDF writer

// Pixel Formats
typedef enum {
	PDFRAS_BITONAL,				// 1-bit per pixel, 0=black
	PDFRAS_GRAY8,				// 8-bit per pixel, 0=black
	PDFRAS_GRAY16,				// 16-bit per pixel, 0=black (under discussion)
	PDFRAS_RGB24,				// 24-bit per pixel, sRGB
	PDFRAS_RGB48,				// 48-bit per pixel, sRGB (under discussion)
} RasterPixelFormat;

// Compression Modes
typedef enum {
	PDFRAS_UNCOMPRESSED,		// uncompressed (/Filter null)
	PDFRAS_JPEG,				// JPEG baseline (DCTDecode)
	PDFRAS_CCITTG4,				// CCITT Group 4 (CCITTFaxDecode)
} RasterCompression;

typedef struct t_pdfrasencoder t_pdfrasencoder;

// create and return a raster PDF encoder.
// apiLevel is the version of this API that the caller is expecting.
// (You can use PDFRAS_API_LEVEL)
// os points to a structure containing various functions and
// handles provided by the caller to the raster encoder.
t_pdfrasencoder* pd_raster_encoder_create(int apiLevel, t_OS *os);

// Set the 'creator' document property.
// Customarily set to the name and version of the creating application.
void pd_raster_set_creator(t_pdfrasencoder *enc, const char* creator);

// Set the resolution for subsequent pages
void pd_raster_set_resolution(t_pdfrasencoder *enc, double xdpi, double ydpi);

// Set the viewing angle for the current page (if any) and subsequent pages.
// The angle is a rotation clockwise in degrees and must be a multiple of 90.
// This angle is initially 0.
void pd_raster_set_rotation(t_pdfrasencoder* enc, int degCW);

// Start encoding a page in the current document.
// If a page is currently open, it is automatically ended before the new page is started.
int pd_raster_encoder_start_page(t_pdfrasencoder* enc, RasterPixelFormat format, RasterCompression comp, int width);

// Set the physical page number for the current page.
// If not set, this property defaults to -1: 'unspecified'.
void pd_raster_set_physical_page_number(t_pdfrasencoder* enc, int phpageno);

// Mark the current page as being a front or back side.
// frontness must be 1 (front side), 0 (back side), or -1 (unspecified)
// If not set, this property defaults to -1 'unspecified'.
void pd_raster_set_page_front(t_pdfrasencoder* enc, int frontness);

// Append a strip h rows high to the current page of the current document.
// The data is len bytes, starting at buf.
// The data must be in the correct pixel format, must have the width given for the page,
// and must be compressed with the specified compression.
// Can be called any number of times to deliver the data for the current page.
// Invalid if no page is open.
// The data is copied byte - for - byte into the output PDF.
// Each row must start on the next byte following the last byte of the preceding row.
// JPEG compressed data must be encoded in the JPEG baseline format.
// Color images must be transformed to YUV space as part of JPEG compression, grayscale images are not transformed.
// CCITT compressed data must be compressed in accordance with the following PDF Optional parameters for the CCITTFaxDecode filter:
// K = -1, EndOfLine=false, EncodedByteAlign=false, BlackIs1=false
int pd_raster_encoder_write_strip(t_pdfrasencoder* enc, int h, const pduint8 *buf, size_t len);

// Finish writing the current page to the current document.
// Invalid if no page is open.
// After this call succeeds, no page is open.
int pd_raster_encoder_end_page(t_pdfrasencoder* enc);

// End the current PDF, finish writing all data to the output.
void pd_raster_encoder_end_document(t_pdfrasencoder* enc);

// Destroy a raster PDF encoder, releasing all associated resources.
// Do not use the enc pointer after this, it is invalid.
void pd_raster_encoder_destroy(t_pdfrasencoder* enc);

#ifdef __cplusplus
}
#endif
#endif
