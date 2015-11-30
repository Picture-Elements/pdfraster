#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "PdfOS.h"
#include "PdfAlloc.h"
#include "PdfDict.h"
#include "PdfStreaming.h"
#include "PdfStandardAtoms.h"
#include "PdfXrefTable.h"
#include "PdfStandardObjects.h"
#include "PdfContentsGenerator.h"

static void myMemSet(void *ptr, pduint8 value, size_t count)
{
	memset(ptr, value, count);
}

static void myMemClear(void *ptr, size_t count)
{
	myMemSet(ptr, 0, count);
}

static int myOutputWriter(const pduint8 *data, pduint32 offset, pduint32 len, void *cookie)
{
	FILE *fp = (FILE *)cookie;
	int i = 0;
	if (!data || !len)
		return 0;
	data += offset;
	fwrite(data, 1, len, fp);
	return len;
}

size_t totalBytes = 0;
static void *mymalloc(size_t bytes)
{
	totalBytes += bytes;
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

static void onimagedataready(t_datasink *sink, void *eventcookie)
{
	pd_datasink_begin(sink);
	pd_datasink_put(sink, _imdata, 0, sizeof(_imdata));
	pd_datasink_end(sink);
	pd_datasink_free(sink);
}

static void pagegen(t_pdcontents_gen *gen, void *cookie)
{
	pd_gen_moveto(gen, 0, 0);
	pd_gen_lineto(gen, 72, 144);
	pd_gen_stroke(gen);
	pd_gen_concatmatrix(gen, 144, 0, 0, 144, 72, 288);
	pd_gen_xobject(gen, PDA_STRIP0);
}

int main(int argc, char **argv)
{
	FILE *fp;
	t_OS os;

	fopen_s(&fp, "output.pdf", "wb");
	os.alloc = mymalloc;
	os.free = free;
	os.memset = myMemSet;
	os.memclear = myMemClear;
	os.allocsys = pd_alloc_sys_new(&os);
	os.writeout = myOutputWriter;
	os.writeoutcookie = fp;

	t_pdoutstream *stm = pd_outstream_new(os.allocsys, &os);

	t_pdxref *xref = pd_xref_new(os.allocsys);

	t_pdvalue catalog = pd_catalog_new(os.allocsys, xref);
	t_pdvalue apage = pd_page_new_simple(os.allocsys, xref, catalog, 612, 792);

	t_pdcontents_gen *gen = pd_contents_gen_new(os.allocsys, pagegen, 0);
	t_pdvalue contents = pd_contents_new(os.allocsys, xref, gen);
	dict_put(apage, PDA_CONTENTS, contents);

	t_pdvalue image = pd_image_new_simple(os.allocsys, xref, onimagedataready, 0, 8, 8, 8, kCompNone, kCCIITTG4, PD_FALSE, kDeviceGray);
	pd_page_add_image(apage, PDA_STRIP0, image);

	pd_catalog_add_page(catalog, apage);

	t_pdvalue info = pd_info_new(os.allocsys, xref, "A Tale of Two Cities", "Charles Dickens", "French Revolution", 0, 0, 0);

	pd_write_pdf_header(stm, "1.4", NULL);
	pd_write_endofdocument(os.allocsys, stm, xref, catalog, info);

	pd_xref_free(xref);
	pd_alloc_sys_free(os.allocsys);
}
