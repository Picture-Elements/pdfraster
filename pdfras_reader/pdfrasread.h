#ifndef _H_pdfrasread
#define _H_pdfrasread
// This API provides an interface for reading byte streams
// that follow the PDF/raster format.
// It does not use or depend on files, or depend on any
// platform services related to files, directories or streams.

#ifdef __cplusplus
extern "C" {
#endif

#include "PdfPlatform.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define RASREAD_API_LEVEL	1

#define PDFRASREAD_VERSION "0.4"
// 0.4	spike	2016.07.21	first formal reporting of compliance failures
//							reader test fails down to 27.
// 0.3	spike	2016.07.19	minor API and internal improvements
// 0.2	spike	2016.07.13	revised PDF-raster marker in trailer dict.
// 0.1	spike	2015.02.11	1st version

// Pixel Formats
typedef enum {
	RASREAD_FORMAT_NULL,		// null value
	RASREAD_BITONAL,			// 1-bit per pixel, 0=black
	RASREAD_GRAY8,				// 8-bit per pixel, 0=black
	RASREAD_GRAY16,				// 16-bit per pixel, 0=black (under discussion)
	RASREAD_RGB24,				// 24-bit per pixel, sRGB
	RASREAD_RGB48,				// 48-bit per pixel, sRGB (under discussion)
} RasterPixelFormat;

// Compression Modes
typedef enum {
	RASREAD_COMPRESSION_NULL,	// null value
	RASREAD_UNCOMPRESSED,		// uncompressed (/Filter null)
	RASREAD_JPEG,				// JPEG baseline (DCTDecode)
	RASREAD_CCITTG4,			// CCITT Group 4 (CCITTFaxDecode)
} RasterCompression;

// function template: read length bytes at offset from source into buffer
typedef size_t (*pdfras_freader)(void *source, pduint32 offset, size_t length, char *buffer);
// function template: close the source
typedef void (*pdfras_fcloser)(void *source);

typedef struct t_pdfrasreader t_pdfrasreader;

// Create a PDF/raster reader in the closed state.
// Return NULL if a reader can't be constructed - typically that can only be a malloc failure.
t_pdfrasreader* pdfrasread_create(int apiLevel, pdfras_freader readfn, pdfras_fcloser closefn);

// Destroy the reader and release all associated resources.
// If open, closes it (and calls the closefn (and ignores any error)).
void pdfrasread_destroy(t_pdfrasreader* reader);

// Open a PDF/raster source for reading.
// If successful, records the source, sets the state to open and returns TRUE.
// Otherwise it returns FALSE.
// This function fails if the reader is already open.
// It also fails if the source does not pass initial parsing/tests for PDF/raster.
// No assumptions are made about the source parameter, it is only stored,
// and passed to the readfn and closefn of the reader.
int pdfrasread_open(t_pdfrasreader* reader, void* source);

// Check if a source is marked as a PDF/raster PDF stream.
// Returns TRUE if a quick header & trailer check passes.
// Returns FALSE otherwise.
int pdfrasread_recognize_source(pdfras_freader fread, void* source);

// Return TRUE if reader has a PDF/raster stream open,
// return FALSE otherwise.
int pdfrasread_is_open(t_pdfrasreader* reader);

// Return the source parameter of the last successful open of this reader.
// Returns NULL if reader is NULL or has never been open.
// To tell if reader is currently open use pdfrasread_is_open, not this.
void* pdfrasread_source(t_pdfrasreader* reader);

// Move a reader to the closed state and disconnect it from associated source.
// This calls the closefn used to create the reader, then resets the
// source associated with this reader to NULL.
// If reader is not open, does nothing and returns TRUE.
// After this call succeeds, pdfrasread_is_open(reader) will return FALSE.
int pdfrasread_close(t_pdfrasreader* reader);

// Return the number of pages in the associated PDF/raster file.
// Only valid if reader is valid (was returned by pdfrasread_create and
// not subsequently destroyed) and is open.
// -1 in case of error.
int pdfrasread_page_count(t_pdfrasreader* reader);

// Return the pixel format of the raster image of page n (indexed from 0)
// Similar restrictions as pdfrasread_page_count.
RasterPixelFormat pdfrasread_page_format(t_pdfrasreader* reader, int n);

// Return the pixel width of the raster image of page n
int pdfrasread_page_width(t_pdfrasreader* reader, int n);

// Return the pixel height of the raster image of page n
int pdfrasread_page_height(t_pdfrasreader* reader, int n);

// Return the resolution in dpi of the raster image of page n
double pdfrasread_page_horizontal_dpi(t_pdfrasreader* reader, int n);
double pdfrasread_page_vertical_dpi(t_pdfrasreader* reader, int n);

// Return the clockwise rotation in degrees to be applied to page n
int pdfrasread_page_rotation(t_pdfrasreader* reader, int n);

// Strip-level access

// Return the number of strips in page p
int pdfrasread_strip_count(t_pdfrasreader* reader, int p);

// Return the maximum raw (compressed) strip size on page p
size_t pdfrasread_max_strip_size(t_pdfrasreader* reader, int p);

// Read the raw (compressed) data of strip s on page p into buffer
// Returns the actual number of bytes read.
// Note that if the the strip is larger than bufsize, no data is read and
// the return value will be 0.
size_t pdfrasread_read_raw_strip(t_pdfrasreader* reader, int p, int s, void* buffer, size_t bufsize);

// detailed error codes
typedef enum {
	READ_OK = 0,
	READ_PAGE_TYPE,					// page dict must have /Type /Page
	READ_PAGE_ROTATION,				// page rotation if present must be an inline non-negative multiple of 90.
	READ_PAGE_MEDIABOX,				// each page dict must have a /MediaBox entry
	READ_RESOURCES,					// each page dictionary must have a /Resources entry (that is a dictionary)
	READ_XOBJECT,					// page resource dictionary must have /XObject entry
	READ_XOBJECT_DICT,				// XObject has to be a dictionary
	READ_XOBJECT_ENTRY,				// only entries in xobject dict must be /strip<n>
	READ_NOT_STRIP_REF,				// strip entry in xobject dict must be an indirect reference to a dictionary
	READ_STRIP_SUBTYPE,				// strip must have /Subtype /Image
	READ_BITSPERCOMPONENT,			// strip must have /BitsPerComponent with inline unsigned integer value.
	READ_STRIP_HEIGHT,				// strip must have /Height entry with inline non-negative integer value
	READ_STRIP_WIDTH,				// strip must have /Width entry with inline non-negative integer value
	READ_STRIP_COLORSPACE,			// strip must have a /Colorspace entry
	READ_INVALID_COLORSPACE,		// colorspace must comply with spec
	READ_STRIP_LENGTH,				// strip must have /Length with non-negative inline integer value

} ReadErrorCode;

#ifdef __cplusplus
}
#endif
#endif
