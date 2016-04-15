// demo_low_level.c - main program for demo_low_level demo app
//
// This test app uses the generic PDF generation functions (not the PdfRaster API)
// to write out a simple one-page PDF, a small raster image with a box around it.
// The output is NOT meant to be a valid PDF/raster file, it's just a vanilla PDF.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "portability.h"

#include "PdfOS.h"
#include "PdfAlloc.h"
#include "PdfDict.h"
#include "PdfStreaming.h"
#include "PdfAtoms.h"
#include "PdfStandardAtoms.h"
#include "PdfXrefTable.h"
#include "PdfImage.h"
#include "PdfStandardObjects.h"
#include "PdfContentsGenerator.h"

// It writes to 'output.pdf' in the current directory.
const char* output_name = "output.pdf";

static void myMemSet(void *ptr, pduint8 value, size_t count)
{
	memset(ptr, value, count);
}

static int myOutputWriter(const pduint8 *data, pduint32 offset, pduint32 len, void *cookie)
{
	FILE *fp = (FILE *)cookie;
	if (!data || !len)
		return 0;
	data += offset;
	fwrite(data, 1, len, fp);
	return len;
}

static void *mymalloc(size_t bytes)
{
	return malloc(bytes);
}

static char _imdata[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,
};

t_pdatom AtomImg0;

static void onimagedataready(t_datasink *sink, void *eventcookie)
{
    UNUSED_FORMAL(eventcookie);
	pd_datasink_put(sink, _imdata, 0, sizeof(_imdata));
}

static void pagegen(t_pdcontents_gen *gen, void *cookie)
{
    UNUSED_FORMAL(cookie);
	pd_gen_moveto(gen, 72-8, 288 - 8);
	pd_gen_lineto(gen, 72 + 144 + 8, 288 - 8);
	pd_gen_lineto(gen, 72 + 144 + 8, 288 + 144 + 8);
	pd_gen_lineto(gen, 72 - 8, 288 + 144 + 8);
	pd_gen_lineto(gen, 72 - 8, 288 - 8);
	pd_gen_stroke(gen);
	pd_gen_concatmatrix(gen, 144, 0, 0, 144, 72, 288);
	pd_gen_xobject(gen, AtomImg0);
}

int main(int argc, char **argv)
{
	FILE *fp;
	t_OS os;

    UNUSED_FORMAL(argc);
    UNUSED_FORMAL(argv);
	remove(output_name);
	fp = fopen(output_name, "wb");
	os.alloc = mymalloc;
	os.free = free;
	os.memset = myMemSet;
	os.allocsys = pd_alloc_new_pool(&os);
	os.writeout = myOutputWriter;
	os.writeoutcookie = fp;

	t_pdoutstream *stm = pd_outstream_new(os.allocsys, &os);

	t_pdxref *xref = pd_xref_new(os.allocsys);
	t_pdatomtable *atoms = pd_atom_table_new(os.allocsys, 100);

	AtomImg0 = pd_atom_intern(atoms, "Img0");

	t_pdvalue catalog = pd_catalog_new(os.allocsys, xref);
	t_pdvalue apage = pd_page_new_simple(os.allocsys, xref, catalog, 612, 792);

	t_pdcontents_gen *gen = pd_contents_gen_new(os.allocsys, pagegen, 0);
	t_pdvalue contents = pd_contents_new(os.allocsys, xref, gen);
	pd_dict_put(apage, PDA_Contents, contents);

	// 8 x 8, 8 bits deep
	double black[3] = { 0.0, 0.0, 0.0 };
	// Should this be D65? [0.9505, 1.0000, 1.0890]?  Does it matter?
	double white[3] = { 1.0, 1.0, 1.0 };
	double gamma = 2.2;
	t_pdvalue calgray = pd_make_calgray_colorspace(os.allocsys, black, white, gamma);
	t_pdvalue image = pd_image_new_simple(os.allocsys, xref, onimagedataready, 0, 8, 8, 8, kCompNone, kCCIITTG4, PD_FALSE, calgray);
	pd_page_add_image(apage, AtomImg0, image);

	pd_catalog_add_page(catalog, apage);

	t_pdvalue info = pd_info_new(os.allocsys, xref);
	pd_dict_put(info, PDA_Title, pdcstrvalue(os.allocsys, "A Tale of Two Cities"));
	pd_dict_put(info, PDA_Author, pdcstrvalue(os.allocsys, "Charles Dickens"));
	pd_dict_put(info, PDA_Subject, pdcstrvalue(os.allocsys, "French Revolution"));

	pd_write_pdf_header(stm, "1.4", NULL);
	pd_write_endofdocument(stm, xref, catalog, info);

	pd_xref_free(xref);
	pd_atom_table_free(atoms);
	pd_alloc_free_pool(os.allocsys);
}
