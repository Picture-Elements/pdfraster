// PdfRaster.c - functions to write PDF/raster
//
#include <assert.h>

#include "PdfRaster.h"
#include "PdfDict.h"
#include "PdfAtoms.h"
#include "PdfStandardAtoms.h"
#include "PdfString.h"
#include "PdfStrings.h"
#include "PdfXrefTable.h"
#include "PdfStandardObjects.h"
#include "PdfImage.h"
#include "PdfArray.h"

typedef struct t_pdfrasencoder {
	t_pdmempool*		pool;
	int					apiLevel;			// caller's specified API level.
	t_pdoutstream*		stm;				// output PDF stream
	void *				writercookie;
	time_t				creationDate;
	t_pdatomtable*		atoms;				// the atom table/dictionary
	// standard document objects
	t_pdxref*			xref;
	t_pdvalue			catalog;
	t_pdvalue			info;
	// optional document objects
	t_pdvalue			srgbColorspace;		// ICCBased sRGB colorspace
	// current page object and related values
	t_pdvalue			currentPage;
	double				xdpi;				// horizontal resolution, pixels/inch
	double				ydpi;				// vertical resolution, pixels/inch
	int					rotation;			// page rotation (degrees clockwise)
	RasterPixelFormat	pixelFormat;		// how pixels are represented
	pdbool				devColor;			// use uncalibrated (device) colorspace
	int					width;				// image width in pixels
	RasterCompression	compression;		// how data is compressed
	int					strips;				// number of strips on current page
	int					height;				// total pixel height of current page
	int					phys_pageno;		// physical page number
	int					page_front;			// front/back/unspecified

} t_pdfrasencoder;

// utility
// Allocate and return a copy of string s
char *pdstrdup(const char* s, struct t_pdmempool *pool)
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


t_pdfrasencoder* pdfr_encoder_create(int apiLevel, t_OS *os)
{
	struct t_pdmempool *pool;

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
		enc->pool = pool;						// associated allocation pool
		enc->apiLevel = apiLevel;				// level of this API assumed by caller
		enc->stm = pd_outstream_new(pool, os);

		enc->xdpi = enc->ydpi = 300;			// default
		enc->rotation = 0;						// default (& redundant)
		enc->compression = PDFRAS_UNCOMPRESSED;	// default
		enc->pixelFormat = PDFRAS_BITONAL;		// default
		// initial atom table
		enc->atoms = pd_atom_table_new(pool, 128);
		// empty cross-reference table:
		enc->xref = pd_xref_new(pool);
		// initial document catalog:
		enc->catalog = pd_catalog_new(pool, enc->xref);
		
		// create 'info' dictionary
		enc->info = pd_info_new(pool, enc->xref);
		// default Producer
		pd_dict_put(enc->info, PDA_Producer, pdcstrvalue(pool, "PdfRaster encoder " PDFRAS_LIBRARY_VERSION));
		// record creation date & time:
		time(&enc->creationDate);
		pd_dict_put(enc->info, PDA_CreationDate, pd_make_time_string(pool, enc->creationDate));
		// we don't modify PDF so there is no ModDate

		// Write the PDF header:
		pd_write_pdf_header(enc->stm, "1.4");
	}
	return enc;
}

void pdfr_encoder_set_creator(t_pdfrasencoder *enc, const char* creator)
{
	pd_dict_put(enc->info, PDA_Creator, pdcstrvalue(enc->pool, creator));
}

void pdfr_encoder_set_author(t_pdfrasencoder *enc, const char* author)
{
	pd_dict_put(enc->info, PDA_Author, pdcstrvalue(enc->pool, author));
}

void pdfr_encoder_set_title(t_pdfrasencoder *enc, const char* title)
{
	pd_dict_put(enc->info, PDA_Title, pdcstrvalue(enc->pool, title));
}

void pdfr_encoder_set_subject(t_pdfrasencoder *enc, const char* subject)
{
	pd_dict_put(enc->info, PDA_Subject, pdcstrvalue(enc->pool, subject));
}

void pdfr_encoder_set_keywords(t_pdfrasencoder *enc, const char* keywords)
{
	pd_dict_put(enc->info, PDA_Keywords, pdcstrvalue(enc->pool, keywords));
}

void f_write_string(t_datasink *sink, void *eventcookie)
{
	const char* xmpdata = (const char*)eventcookie;
	pd_datasink_put(sink, xmpdata, 0, pdstrlen(xmpdata));
}

void pdfr_encoder_get_creation_date(t_pdfrasencoder *enc, time_t *t)
{
	*t = enc->creationDate;
}

void pdfr_encoder_write_document_xmp(t_pdfrasencoder *enc, const char* xmpdata)
{
	t_pdvalue xmpstm = pd_metadata_new(enc->pool, enc->xref, f_write_string, (void*)xmpdata);
	// flush the metadata stream to output immediately
	pd_write_reference_declaration(enc->stm, xmpstm);
	pd_dict_put(enc->catalog, PDA_Metadata, xmpstm);
}

void pdfr_encoder_write_page_xmp(t_pdfrasencoder *enc, const char* xmpdata)
{
	t_pdvalue xmpstm = pd_metadata_new(enc->pool, enc->xref, f_write_string, (void*)xmpdata);
	// flush the metadata stream to output immediately
	pd_write_reference_declaration(enc->stm, xmpstm);
	pd_dict_put(enc->currentPage, PDA_Metadata, xmpstm);
}

void pdfr_encoder_set_resolution(t_pdfrasencoder *enc, double xdpi, double ydpi)
{
	enc->xdpi = xdpi;
	enc->ydpi = ydpi;
}

// Set the rotation for current and subsequent pages.
// Values that are not multiple of 90 are *ignored*.
// Valid values are mapped into the range 0, 90, 180, 270.
void pdfr_encoder_set_rotation(t_pdfrasencoder* enc, int degCW)
{
	while (degCW < 0) degCW += 360;
	if (degCW % 90 == 0) {
		enc->rotation = degCW % 360;
	}
}

void pdfr_encoder_set_pixelformat(t_pdfrasencoder* enc, RasterPixelFormat format)
{
	enc->pixelFormat = format;
}

void pdfr_encoder_set_compression(t_pdfrasencoder* enc, RasterCompression comp)
{
	enc->compression = comp;
}

void pdfr_encoder_set_device_colorspace(t_pdfrasencoder* enc, int devColor)
{
	enc->devColor = (devColor != 0);
}

int pdfr_encoder_start_page(t_pdfrasencoder* enc, int width)
{
	if (IS_DICT(enc->currentPage)) {
		pdfr_encoder_end_page(enc);
		assert(IS_NULL(enc->currentPage));
	}
	enc->width = width;
	enc->strips = 0;				// number of strips written to current page
	enc->height = 0;				// height of current page so far

	// per-page metadata:
	enc->phys_pageno = -1;			// unspecified
	enc->page_front = -1;			// unspecified

	double W = width / enc->xdpi * 72.0;
	// Start a new page (of unknown height)
	enc->currentPage = pd_page_new_simple(enc->pool, enc->xref, enc->catalog, W, 0);
	assert(IS_REFERENCE(enc->currentPage));

	return 0;
}

void pdfr_encoder_set_physical_page_number(t_pdfrasencoder* enc, int phpageno)
{
	enc->phys_pageno = phpageno;
}

void pdfr_encoder_set_page_front(t_pdfrasencoder* enc, int frontness)
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
	pd_datasink_put(sink, pinfo->data, 0, pinfo->count);
}

t_pdvalue pdfr_encoder_get_srgb_colorspace(t_pdfrasencoder* enc)
{
	// Just use a single sRGB colorspace for the entire document,
	// created when first needed.
	if (IS_ERR(enc->srgbColorspace)) {
		t_pdvalue srgb = pd_make_srgb_colorspace(enc->pool, enc->xref);
		// flush the colorspace profile to the output
		t_pdvalue profile = pd_array_get(srgb.value.arrvalue, 1);
		pd_write_reference_declaration(enc->stm, profile);
		enc->srgbColorspace = srgb;
	}
	return enc->srgbColorspace;
}

t_pdvalue pdfr_encoder_get_calgray_colorspace(t_pdfrasencoder* enc)
{
	double black[3] = { 0.0, 0.0, 0.0 };
	// Should this be D65? [0.9505, 1.0000, 1.0890]?  Does it matter?
	double white[3] = { 1.0, 1.0, 1.0 };
	double gamma = 2.25;
	return pd_make_calgray_colorspace(enc->pool, black, white, gamma);
}

t_pdvalue pdfr_encoder_get_colorspace(t_pdfrasencoder* enc)
{
	switch (enc->pixelFormat) {
	case PDFRAS_RGB24:
	case PDFRAS_RGB48:
		// unless told to use device colorspace, use ICC profile colorspace
		if (enc->devColor) {
			return pdatomvalue(PDA_DeviceRGB);
		}
		else {
			return pdfr_encoder_get_srgb_colorspace(enc);
		}
	case PDFRAS_GRAY8:
	case PDFRAS_GRAY16:
	case PDFRAS_BITONAL:
		// unless told to use device grayscale, use calibrated grayscale
		if (enc->devColor) {
			return pdatomvalue(PDA_DeviceGray);
		}
		else {
			return pdfr_encoder_get_calgray_colorspace(enc);
		}
	default:
		break;
	} // switch
	return pderrvalue();
}

int pdfr_encoder_write_strip(t_pdfrasencoder* enc, int rows, const pduint8 *buf, size_t len)
{
	t_pdvalue colorspace = pdfr_encoder_get_colorspace(enc);
	char stripname[12] = "Strip";

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
	t_stripinfo stripinfo;
	stripinfo.data = buf;
	stripinfo.count = len;
	t_pdvalue image = pd_image_new_simple(enc->pool, enc->xref, onimagedataready, &stripinfo,
		enc->width, rows, bitsPerComponent,
		comp,
		kCCIITTG4, PD_FALSE,			// ignored unless compression is CCITT
		colorspace);
	// get a reference to this (strip) image
	t_pdvalue imageref = pd_xref_makereference(enc->xref, image);
	pd_strcpy(stripname + 5, ELEMENTS(stripname) - 5, pditoa(enc->strips));
	// turn strip name into an atom
	t_pdatom strip = pd_atom_intern(enc->atoms, stripname);
	// add the image to the resources of the current page, with the given name
	pd_page_add_image(enc->currentPage, strip, imageref);
	// flush the image stream
	pd_write_reference_declaration(enc->stm, imageref);
	// adjust total page height:
	enc->height += rows;
	// increment strip count:
	enc->strips++;

	return 0;
}

int pdfr_encoder_get_page_height(t_pdfrasencoder* enc)
{
	return enc->height;
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
	// vertical offset starts at top of page
	double ty = H;
	pdbool succ;
	t_pdvalue res = pd_dict_get(enc->currentPage, PDA_Resources, &succ);
	t_pdvalue xobj = pd_dict_get(res, PDA_XObject, &succ);
	for (int n = 0; n < enc->strips; n++) {
		char stripNname[12] = "Strip";
		pd_strcpy(stripNname + 5, ELEMENTS(stripNname) - 5, pditoa(n));
		// turn strip name into an atom
		t_pdatom stripNatom = pd_atom_intern(enc->atoms, stripNname);
		// find the strip Image resource
		t_pdvalue img = pd_dict_get(xobj, stripNatom, &succ);
		// get its /Height
		t_pdvalue striph = pd_dict_get(img, PDA_Height, &succ);
		// calculate height in Points
		double SH = striph.value.intvalue * 72.0 / enc->ydpi;
		pd_gen_gsave(gen);
		pd_gen_concatmatrix(gen, W, 0, 0, SH, tx, ty-SH);
		pd_gen_xobject(gen, stripNatom);
		pd_gen_grestore(gen);
		ty -= SH;
	}
}

static void update_media_box(t_pdfrasencoder* enc, t_pdvalue page)
{
	pdbool success = PD_FALSE;
	t_pdvalue box = pd_dict_get(page, PDA_MediaBox, &success);
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
		t_pdvalue privDict = pd_dict_new(enc->pool, 2);
		if (enc->phys_pageno >= 0) {
			pd_dict_put(privDict, PDA_PhysicalPageNumber, pdintvalue(enc->phys_pageno));
		}
		if (enc->page_front >= 0) {
			pd_dict_put(privDict, PDA_FrontSide, pdboolvalue(enc->page_front == 1));
		}
		t_pdvalue appDataDict = pd_dict_new(enc->pool, 2);
		pd_dict_put(appDataDict, PDA_LastModified, modTime);
		pd_dict_put(appDataDict, PDA_Private, privDict);
		t_pdvalue pieceInfo = pd_dict_new(enc->pool, 2);
		pd_dict_put(pieceInfo, PDA_PDFRaster, appDataDict);
		pd_dict_put(enc->currentPage, PDA_PieceInfo, pieceInfo);
		pd_dict_put(enc->currentPage, PDA_LastModified, modTime);
	}
}

int pdfr_encoder_end_page(t_pdfrasencoder* enc)
{
	if (!IS_NULL(enc->currentPage)) {
		// create a content generator
		t_pdcontents_gen *gen = pd_contents_gen_new(enc->pool, content_generator, enc);
		// create contents object (stream)
		t_pdvalue contents = pd_xref_makereference(enc->xref, pd_contents_new(enc->pool, enc->xref, gen));
		// flush (write) the contents stream
		pd_write_reference_declaration(enc->stm, contents);
		// add the contents to the current page
		pd_dict_put(enc->currentPage, PDA_Contents, contents);
		// update the media box (we didn't really know the height until now)
		update_media_box(enc, enc->currentPage);
		if (enc->rotation != 0) {
			pd_dict_put(enc->currentPage, PDA_Rotate, pdintvalue(enc->rotation));
		}
		// metadata - add to page if any is specified
		write_page_metadata(enc);
		// flush (write) the current page
		pd_write_reference_declaration(enc->stm, enc->currentPage);
		// add the current page to the catalog (page tree)
		pd_catalog_add_page(enc->catalog, enc->currentPage);
		// done with current page:
		enc->currentPage = pdnullvalue();
	}
	return 0;
}

void pdfr_encoder_end_document(t_pdfrasencoder* enc)
{
	pdfr_encoder_end_page(enc);
	pd_write_endofdocument(enc->stm, enc->xref, enc->catalog, enc->info);
	pd_xref_free(enc->xref); enc->xref = NULL;
	pd_atom_table_free(enc->atoms); enc->atoms = NULL;
}

void pdfr_encoder_destroy(t_pdfrasencoder* enc)
{
	if (enc) {
		struct t_pdmempool *pool = enc->pool;
		pd_alloc_free_pool(pool);
	}
}

