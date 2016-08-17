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

// Error categories
// All levels except INFO and WARNING indicate that the current API call will fail.
#define	REPORTING_INFO      0		// useful to know but not bad news.
#define	REPORTING_WARNING   1		// a potential problem - but execution can continue.
#define	REPORTING_COMPLIANCE 2	    // a violation of the PDF/raster specification was detected.
#define	REPORTING_API       3		// an invalid request was made to this API.
#define	REPORTING_MEMORY    4		// memory allocation failed.
#define	REPORTING_IO        5		// PDF read or write failed unexpectedly.
#define	REPORTING_LIMIT     6		// a built-in limitation of this library was exceeded.
#define	REPORTING_INTERNAL  7		// an 'impossible' internal state has been detected.
#define	REPORTING_OTHER     8		// none of the above, and the current API call cannot complete.

typedef struct t_pdfrasreader t_pdfrasreader;

// function template: read length bytes into buffer, starting at offset in source.
typedef size_t (*pdfras_freader)(void *source, pduint32 offset, size_t length, char *buffer);

// function template: return the size of a source
typedef pduint32 (*pdfras_fsizer)(void* source);

// function template: close a source
typedef void (*pdfras_fcloser)(void *source);

// function template: error/warning handler
typedef int(*pdfras_err_handler)(t_pdfrasreader* reader, int level, int code, pduint32 offset);

// Create a PDF/raster reader in the closed state.
// Return NULL if a reader can't be constructed - typically that can only be a malloc failure.
t_pdfrasreader* pdfrasread_create(int apiLevel, pdfras_freader readfn, pdfras_fsizer sizefn, pdfras_fcloser closefn);

// Destroy the reader and release all associated resources.
// If open, closes it (and calls the closefn (and ignores any error)).
void pdfrasread_destroy(t_pdfrasreader* reader);

// Return the version of this library, as a string
// of the form a.b.c.d (config)
// where a.b.c.d is a conventional 4-field version number,
// and config is the word DEBUG or RELEASE, possibly followed by
// some kind of platform description like WIN32/x86
const char* pdfrasread_lib_version(void);

// Set the global error handler for this library.
// There is just one shared global error handler for each instance of the library.
// By default, all errors are passed to the global error handler.
//
// Passing errhandler = NULL is valid, and restores the default error handler.
// See: pdfrasread_default_error_handler below.
//
// If an error is associated with a valid t_pdfrasread* reader, that reader
// will be passed to the handler, otherwise NULL is passed.
// See pdfrasread_set_error_handler to override error handling for individual readers.
void pdfrasread_set_global_error_handler(pdfras_err_handler errhandler);

// Attach an error-handler to a PDF/raster reader.
// Note the first parameter is a t_pdfrasreader*, not a void* source like the readfn and closefn.
//
// Passing errhandler = NULL is valid, and restores the default error handler.
//
// The handler SHOULD RETURN 0 even though the return value is currently ignored.
void pdfrasread_set_error_handler(t_pdfrasreader* reader, pdfras_err_handler errhandler);

// The default error-handler, used if you do not specify another handler.
// It prints to stderr a somewhat descriptive 1-line message that starts with 
int pdfrasread_default_error_handler(t_pdfrasreader* reader, int level, int code, pduint32 offset);

// Set *pmajor and *pminor to the highest PDF/raster version this library supports/understands.
// Note: The library will refuse to process a file with a higher major version.
// Note: The library will try to read any file with an equal or lower major version.
//       (but it will call the error handler with a warning.)
void pdfrasread_get_highest_pdfr_version(t_pdfrasreader* reader, int* pmajor, int* pminor);

// Open a PDF/raster source for reading.
// If successful, records the source, sets the state to open and returns TRUE.
// Otherwise it returns FALSE.
// This function fails if the reader is already open.
// It also fails if the source does not pass initial parsing/tests for PDF/raster.
// No assumptions are made about the source parameter, it is only stored,
// and passed to the readfn and closefn of the reader.
int pdfrasread_open(t_pdfrasreader* reader, void* source);

// Check if a source passes a quick validation as a PDF/raster stream.
// Returns TRUE if a quick header & trailer check passes.
// Returns FALSE otherwise.
// It's OK to pass NULL for pmajor or pminor, they'll be ignored.
// If pmajor is not NULL, *pmajor is always set to something.
// Ditto for pminor.
// Returns FALSE with *pmajor = -1 if source is not even PDF.
// Returns FALSE with *pmajor = 0 if source is PDF, but not PDF/raster.
// Returns FALSE if the major version is above what this library is
// designed to handle.
// Returns TRUE if the major version is OK, EVEN IF the major.minor is
// above what this library is compiled for.
int pdfrasread_recognize_source(t_pdfrasreader* reader, void* source, int* pmajor, int* pminor);

// Return TRUE if reader has a PDF/raster stream open,
// return FALSE otherwise.
int pdfrasread_is_open(t_pdfrasreader* reader);

// Return the source parameter of the last successful open of this reader.
// Returns NULL if reader is NULL or it has never been open.
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
// TODO: assign hard codes to all, so they can't change accidentally
// and so people can look 'em up.
typedef enum {
	READ_OK = 0,
    READ_API_BAD_READER,            // an API function was called with an invalid reader
    READ_API_APILEVEL,              // pdfrasread_create called with apiLevel < 1 or > what library supports.
    READ_API_ALREADY_OPEN,          // function called with a reader that was already open.
    READ_INTERNAL_XREF_SIZE,        // internal build/compilation error: sizeof(xref_entry) != 20 bytes
    READ_MEMORY_MALLOC,             // malloc returned NULL - insufficient memory (or heap corrupt)
    READ_FILE_EOF_MARKER,           // %%EOF not found near end of file (prob. not a PDF)
    READ_FILE_STARTXREF,            // startxref not found near end of file (so prob. not a PDF)
    READ_FILE_BAD_STARTXREF,        // startxref found - but invalid syntax or value
    READ_FILE_PDFRASTER_TAG,        // %PDF-raster tag not found near end of file
    READ_FILE_BAD_TAG,              // %PDF-raster- not followed by valid <int>.<int><eol>
    READ_FILE_TOO_MAJOR,            // file's PDF-raster major version is above what this library supports.
    READ_FILE_TOO_MINOR,            // this file's PDF-raster minor version is above what this library understands.
    READ_PAGE_TYPE,					// page dict must have /Type /Page
	READ_PAGE_ROTATION,				// page rotation if present must be an inline non-negative multiple of 90.
	READ_PAGE_MEDIABOX,				// each page dict must have a /MediaBox entry
    READ_MEDIABOX_ARRAY,            // MediaBox value must be an array
    READ_MEDIABOX_ELEMENTS,         // MediaBox must contain 4 numbers: [0 0 w h]
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
    READ_ERROR_CODE_COUNT
} ReadErrorCode;

#ifdef __cplusplus
}
#endif
#endif
