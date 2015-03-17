// PdfRaster.c - functions to write PDF/raster
//
#include <assert.h>

#include "PdfRaster.h"
#include "PdfDict.h"
#include "PdfAtoms.h"
#include "PdfStandardAtoms.h"
#include "PdfString.h"
#include "PdfXrefTable.h"
#include "PdfStandardObjects.h"
#include "PdfArray.h"

// Version of the file format we 
#define PDFRASTER_SPEC_VERSION "1.0"

typedef struct t_pdfrasencoder {
	t_pdallocsys*		pool;
	int					apiLevel;			// caller's specified API level.
	t_pdoutstream*		stm;				// output PDF stream
	void *				writercookie;
	// document info
	char*				keywords;
	char*				creator;			// name of application creating the PDF
	char*				producer;			// name of PDF writing module
	// internal PDF structures
	t_pdxref*			xref;
	t_pdvalue			catalog;
	// current page object and related values
	t_pdvalue			currentPage;
	double				xdpi;				// horizontal resolution, pixels/inch
	double				ydpi;				// vertical resolution, pixels/inch
	int					rotation;			// page rotation (degrees clockwise)
	RasterPixelFormat	pixelFormat;		// how pixels are represented
	int					width;				// image width in pixels
	RasterCompression	compression;		// how data is compressed
	int					height;				// total pixel height of current page
	int					phys_pageno;		// physical page number
	int					page_front;			// front/back/unspecified

} t_pdfrasencoder;

// utility
// Allocate and return a copy of string s
char *pdstrdup(const char* s, struct t_pdallocsys *pool)
{
	size_t n = pdstrlen(s)+1;
	char* t = (char*)pd_alloc(pool, n);
	if (t) {
		char *p = t;
		while (n--) {
			*p++ = *s++;
		}
	}
	return t;
}


t_pdfrasencoder* pd_raster_encoder_create(int apiLevel, t_OS *os)
{
	struct t_pdallocsys *pool;

	if (apiLevel < 1) {
		// TODO: report error
		// invalid apiLevel parameter value
		return NULL;
	}
	if (apiLevel > 1) {
		// TODO: report error
		// Caller was compiled for a later version of this API
		return NULL;
	}
	assert(os);
	pool = os->allocsys;
	assert(pool);

	t_pdfrasencoder *enc = (t_pdfrasencoder *)pd_alloc(pool, sizeof(t_pdfrasencoder));
	if (enc)
	{
		enc->pool = pool;
		enc->apiLevel = apiLevel;
		enc->stm = pd_outstream_new(pool, os);
		// fill in various defaults
		enc->keywords = NULL;
		enc->creator = NULL;
		enc->producer = "PdfRaster encoder " PDFRAS_LIBRARY_VERSION;
		// default resolution
		enc->xdpi = enc->ydpi = 42;

		enc->xref = pd_xref_new(pool);
		enc->catalog = pd_catalog_new(pool, enc->xref);
		// Write the PDF header, with the PDF/raster 2nd line comment:
		pd_write_pdf_header(enc->stm, "1.4", "%\xAE\xE2\x9A\x86" "er-" PDFRASTER_SPEC_VERSION "\n");
	}
	return enc;
}

void pd_raster_set_creator(t_pdfrasencoder *enc, const char* creator)
{
	if (enc->creator) {
		pd_free(enc->pool, enc->creator);
		enc->creator = NULL;
	}
	if (creator) {
		enc->creator = pdstrdup(creator, enc->pool);
	}
}

void pd_raster_set_resolution(t_pdfrasencoder *enc, double xdpi, double ydpi)
{
	enc->xdpi = xdpi;
	enc->ydpi = ydpi;
}

// Set the rotation for current and subsequent pages.
// Values that are not multiple of 90 are *ignored*.
// Valid values are mapped into the range 0, 90, 180, 270.
void pd_raster_set_rotation(t_pdfrasencoder* enc, int degCW)
{
	while (degCW < 0) degCW += 360;
	if (degCW % 90 == 0) {
		enc->rotation = degCW % 360;
	}
}

int pd_raster_encoder_start_page(t_pdfrasencoder* enc, RasterPixelFormat format, RasterCompression comp, int width)
{
	if (IS_DICT(enc->currentPage)) {
		pd_raster_encoder_end_page(enc);
		assert(IS_NULL(enc->currentPage));
	}
	enc->pixelFormat = format;
	enc->compression = comp;
	enc->width = width;
	enc->height = 0;				// unknown until strips are written

	// per-page metadata:
	enc->phys_pageno = -1;			// unspecified
	enc->page_front = -1;			// unspecified

	double W = width / enc->xdpi * 72.0;
	// Start a new page (of unknown height)
	enc->currentPage = pd_page_new_simple(enc->pool, enc->xref, enc->catalog, W, 0);
	assert(IS_REFERENCE(enc->currentPage));

	return 0;
}

void pd_raster_set_physical_page_number(t_pdfrasencoder* enc, int phpageno)
{
	enc->phys_pageno = phpageno;
}

void pd_raster_set_page_front(t_pdfrasencoder* enc, int frontness)
{
	enc->page_front = frontness;
}


typedef struct {
	const pduint8* data;
	size_t count;
} t_stripinfo;

static void onimagedataready(t_datasink *sink, void *eventcookie)
{
	t_stripinfo* pinfo = (t_stripinfo*)eventcookie;
	pd_datasink_begin(sink);
	pd_datasink_put(sink, pinfo->data, 0, pinfo->count);
	pd_datasink_end(sink);
	pd_datasink_free(sink);
}

int pd_raster_encoder_write_strip(t_pdfrasencoder* enc, int rows, const pduint8 *buf, size_t len)
{
	e_ColorSpace colorspace = enc->pixelFormat >= PDFRAS_RGB24 ? kDeviceRgb : kDeviceGray;
	e_ImageCompression comp;
	switch (enc->compression) {
	case PDFRAS_CCITTG4:
		comp = kCompCCITT;
		break;
	case PDFRAS_JPEG:
		comp = kCompDCT;
		break;
	default:
		comp = kCompNone;
		break;
	}
	int bitsPerComponent;
	switch (enc->pixelFormat) {
	case PDFRAS_BITONAL:
		bitsPerComponent = 1;
		break;
	case PDFRAS_GRAY16:
	case PDFRAS_RGB48:
		bitsPerComponent = 16;
		break;
	default:
		bitsPerComponent = 8;
		break;
	} // switch
	t_stripinfo stripinfo = { buf, len };
	t_pdvalue image = pd_image_new_simple(enc->pool, enc->xref, onimagedataready, &stripinfo,
		enc->width, rows, bitsPerComponent,
		comp,
		kCCIITTG4, PD_FALSE,			// ignored unless compression is CCITT
		colorspace);
	// get a reference to this (strip) image
	t_pdvalue imageref = pd_xref_makereference(enc->xref, image);
	// add the image to the resources of the current page
	pd_page_add_image(enc->currentPage, PDA_STRIP0, imageref);
	// flush the image stream
	pd_write_pdreference_declaration(enc->stm, imageref.value.refvalue);
	enc->height += rows;

	return 0;
}

// callback to generate the content text for a page.
// it draws the strips of the page in order from top to bottom.
static void content_generator(t_pdcontents_gen *gen, void *cookie)
{
	t_pdfrasencoder* enc = (t_pdfrasencoder*)cookie;
	// compute width & height of page in PDF points
	double W = enc->width / enc->xdpi * 72.0;
	double H = enc->height / enc->ydpi * 72.0;
	// horizontal (x) offset is always 0 - flush to left edge.
	double tx = 0;
	// vertical offset starts at 0? Top of page?
	// TODO: ty needs to be stepped for each strip
	double ty = 0;
	// TODO: draw all strips not just the first
	pd_gen_gsave(gen);
	pd_gen_concatmatrix(gen, W, 0, 0, H, tx, ty);
	pd_gen_xobject(gen, PDA_STRIP0);
	pd_gen_grestore(gen);
}

static void update_media_box(t_pdfrasencoder* enc, t_pdvalue page)
{
	pdbool success = PD_FALSE;
	t_pdvalue box = dict_get(page, PDA_MEDIABOX, &success);
	assert(success);
	assert(IS_ARRAY(box));
	double W = enc->width / enc->xdpi * 72.0;
	double H = enc->height / enc->ydpi * 72.0;
	pd_array_set(box.value.arrvalue, 2, pdfloatvalue(W));
	pd_array_set(box.value.arrvalue, 3, pdfloatvalue(H));
}

static void write_page_metadata(t_pdfrasencoder* enc)
{
	if (enc->page_front >= 0 || enc->phys_pageno >= 0) {
		t_pdvalue modTime = pd_make_now_string(enc->pool);
		t_pdvalue privDict = dict_new(enc->pool, 2);
		if (enc->phys_pageno >= 0) {
			dict_put(privDict, PDA_PHYSICALPAGENUMBER, pdintvalue(enc->phys_pageno));
		}
		if (enc->page_front >= 0) {
			dict_put(privDict, PDA_FRONTSIDE, pdboolvalue(enc->page_front == 1));
		}
		t_pdvalue appDataDict = dict_new(enc->pool, 2);
		dict_put(appDataDict, PDA_LASTMODIFIED, modTime);
		dict_put(appDataDict, PDA_PRIVATE, privDict);
		t_pdvalue pieceInfo = dict_new(enc->pool, 2);
		dict_put(pieceInfo, PDA_PDFRASTER, appDataDict);
		dict_put(enc->currentPage, PDA_PIECEINFO, pieceInfo);
		dict_put(enc->currentPage, PDA_LASTMODIFIED, modTime);
	}
}

int pd_raster_encoder_end_page(t_pdfrasencoder* enc)
{
	if (!IS_NULL(enc->currentPage)) {
		// create a content generator
		t_pdcontents_gen *gen = pd_contents_gen_new(enc->pool, content_generator, enc);
		// create contents object (stream)
		t_pdvalue contents = pd_xref_makereference(enc->xref, pd_contents_new(enc->pool, enc->xref, gen));
		// flush (write) the contents stream
		pd_write_pdreference_declaration(enc->stm, contents.value.refvalue);
		// add the contents to the current page
		dict_put(enc->currentPage, PDA_CONTENTS, contents);
		// update the media box (we didn't really know the height until now)
		update_media_box(enc, enc->currentPage);
		if (enc->rotation != 0) {
			dict_put(enc->currentPage, PDA_ROTATE, pdintvalue(enc->rotation));
		}
		// metadata - add to page if any is specified
		write_page_metadata(enc);
		// flush (write) the current page
		pd_write_pdreference_declaration(enc->stm, enc->currentPage.value.refvalue);
		// add the current page to the catalog (page tree)
		pd_catalog_add_page(enc->catalog, enc->currentPage);
		// done with current page:
		enc->currentPage = pdnullvalue();
	}
	return 0;
}

void pd_raster_encoder_end_document(t_pdfrasencoder* enc)
{
	pd_raster_encoder_end_page(enc);
	t_pdvalue info = pd_info_new(enc->pool, enc->xref, "A Tale of Two Cities", "Charles Dickens", "French Revolution", enc->keywords, enc->creator, enc->producer);
	pd_write_endofdocument(enc->pool, enc->stm, enc->xref, enc->catalog, info);
	pd_xref_free(enc->xref); enc->xref = NULL;
}

void pd_raster_encoder_destroy(t_pdfrasencoder* enc)
{
	if (enc) {
		struct t_pdallocsys *pool = enc->pool;
		pd_alloc_sys_free(pool);
	}
}

