// raster_encoder_demo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PdfRaster.h"

#include "bw_ccitt_data.h"
#include "color_page.h"
#include "gray8_page.h"

#define OUTPUT_FILENAME "raster.pdf"

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

// tiny gray8 image, 8x8:
static char _imdata[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};

// slightly larger gray16 image:
static pduint16 deepGrayData[64 * 512];

static char bitonalData[((850 + 7) / 8) * 1100];

// 24-bit RGB image data
struct { unsigned char R, G, B; } colorData[175 * 100];

static char XMP_metadata[4096] = "\
<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n\
<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n\
  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\
	<rdf:Description rdf:about=\"\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\
	    <dc:format>application/pdf</dc:format>\
	</rdf:Description>\
	<rdf:Description rdf:about=\"\" xmlns:xap=\"http://ns.adobe.com/xap/1.0/\">\
	    <xap:CreateDate>2013-08-27T10:28:38+07:00</xap:CreateDate>\
		<xap:ModifyDate>2013-08-27T10:28:38+07:00</xap:ModifyDate>\
		<xap:CreatorTool>raster_encoder_demo 1.0</xap:CreatorTool>\
	</rdf:Description>\
	<rdf:Description rdf:about=\"\" xmlns:pdf=\"http://ns.adobe.com/pdf/1.3/\">\
		<pdf:Producer>PdfRaster encoder 0.8</pdf:Producer>\
	</rdf:Description>\
	<rdf:Description rdf:about=\"\" xmlns:xapMM=\"http://ns.adobe.com/xap/1.0/mm/\"><xapMM:DocumentID>uuid:42646CE2-2A6C-482A-BC04-030FDD35E676</xapMM:DocumentID>\
	</rdf:Description>\
"
//// Tag file as PDF/A-1b
//"\
//	<rdf:Description rdf:about=\"\" xmlns:pdfaid=\"http://www.aiim.org/pdfa/ns/id/\" pdfaid:part=\"1\" pdfaid:conformance=\"B\">\
//	</rdf:Description>\
//"
"\
  </rdf:RDF>\
</x:xmpmeta>\n\
\n\
<?xpacket end=\"w\"?>\
";

void set_xmp_create_date(char* xmp, time_t t)
{
	const char* CREATEDATE = "<xap:CreateDate>";
	const char* MODIFYDATE = "<xap:ModifyDate>";
	char* p = strstr(xmp, CREATEDATE);
	if (p) {
		p += strlen(CREATEDATE);
		char* createDate = p;
		// Format the time per XMP basic metadata
		// YYYY-MM-DDThh:mm:ssTZD
		struct tm *tmp = localtime(&t);
		// As a side effect, localtime sets global 'timezone'
		// to offset from localtime to UTC in seconds.
		// We want the offset FROM UTC to local, and in minutes:
		// "A PLUS SIGN as the value of the O field signifies that local time is at or later than UT,
		// a HYPHEN - MINUS signifies that local time is earlier than UT
		long UTCoff = -timezone / 60;
		char chSign = '+';
		if (UTCoff < 0) {
			chSign = '-'; UTCoff = -UTCoff;
		}
		// Note - strftime is in theory affected by the current locale but
		// the conversion specifiers we use here are locale-independent.
		strftime(p, 20, "%Y-%m-%dT%H:%M:%S", tmp);
		// append offset to local time from UTC in the form <sign>hh:mm
		p += pdstrlen(p);
		sprintf(p, "%c%02d:%02d", chSign, UTCoff / 60, UTCoff % 60);
		p += pdstrlen(p);
		// hack alert...  put back the following '<' that
		// was clobbered by the trailing NUL of the sprintf:
		*p = '<';
		size_t datelen = p - createDate;

		p = strstr(xmp, MODIFYDATE);
		if (p) {
			p += pdstrlen(MODIFYDATE);
			memmove(p, createDate, datelen);
		}
	}
}


void generate_image_data()
{
	// generate bitonal page data
	for (int i = 0; i < sizeof bitonalData; i++) {
		int y = (i / 107);
		int b = (i % 107);
		if ((y % 100) == 0) {
			bitonalData[i] = 0xAA;
		}
		else if ((b % 12) == 0 && (y & 1)) {
			bitonalData[i] = 0x7F;
		}
		else {
			bitonalData[i] = 0xff;
		}
	}
	// generate 16-bit grayscale data
	// 64 columns, 512 rows
	for (int i = 0; i < 64 * 512; i++) {
		int y = (i / 64);
		unsigned value = 65535 - (y * 65535 / 511);
		pduint8* pb = (pduint8*)(deepGrayData + i);
		pb[0] = value / 256;
		pb[1] = value % 256;
	}

	// generate RGB data
	for (int i = 0; i < (175 * 100); i++) {
		int y = (i / 175);
		int x = (i % 175);
		colorData[i].R = x * 255 / 175;
		colorData[i].G = y * 255 / 100;
		colorData[i].B = (x + y) * 255 / (100 + 175);
	}
} // generate_image_data

// write 8.5 x 11 bitonal page 100 DPI with a light dotted grid
void write_bitonal_uncomp_page(t_pdfrasencoder* enc)
{
	pdfr_encoder_set_resolution(enc, 100.0, 100.0);
	pdfr_encoder_start_page(enc, 850);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_BITONAL);
	pdfr_encoder_set_compression(enc, PDFRAS_UNCOMPRESSED);
	pdfr_encoder_write_strip(enc, 1100, bitonalData, sizeof bitonalData);
	pdfr_encoder_end_page(enc);
}

int write_bitonal_uncompressed_file(t_OS os, const char *filename)

{
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	// Construct a raster PDF encoder
	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_subject(enc, "BW 1-bit Uncompressed sample output");

	write_bitonal_uncomp_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_bitonal_ccitt_page(t_pdfrasencoder* enc)
{
	// Next page: CCITT-compressed B&W 300 DPI US Letter (scanned)
	pdfr_encoder_set_resolution(enc, 300.0, 300.0);
	pdfr_encoder_start_page(enc, 2521);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_BITONAL);
	pdfr_encoder_set_compression(enc, PDFRAS_CCITTG4);
	pdfr_encoder_write_strip(enc, 3279, bw_ccitt_data, sizeof bw_ccitt_data);
	pdfr_encoder_end_page(enc);
}

int write_bitonal_ccitt_file(t_OS os, const char *filename)
{
	// Write a file: CCITT-compressed B&W 300 DPI US Letter (scanned)
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_author(enc, "Willy Codewell");
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_keywords(enc, "raster bitonal CCITT");
	pdfr_encoder_set_subject(enc, "BW 1-bit CCITT-G4 compressed sample output");

	write_bitonal_ccitt_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_gray8_uncomp_page(t_pdfrasencoder* enc)
{
	// 8-bit grayscale, uncompressed, 4" x 5.5" at 2.0 DPI
	pdfr_encoder_set_resolution(enc, 2.0, 2.0);
	// start a new page:
	pdfr_encoder_start_page(enc, 8);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_GRAY8);
	pdfr_encoder_set_compression(enc, PDFRAS_UNCOMPRESSED);
	// write a strip of raster data to the current page
	// 11 rows high
	pdfr_encoder_write_strip(enc, 11, _imdata, sizeof _imdata);
	// the page is done
	pdfr_encoder_end_page(enc);
}

int write_gray8_uncompressed_file(t_OS os, const char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_subject(enc, "GRAY8 Uncompressed sample output");

	pdfr_encoder_write_page_xmp(enc, XMP_metadata);

	write_gray8_uncomp_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_gray8_jpeg_page(t_pdfrasencoder* enc)
{
	// 4" x 5.5" at 2.0 DPI
	pdfr_encoder_set_resolution(enc, 100.0, 100.0);
	// start a new page:
	pdfr_encoder_start_page(enc, 850);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_GRAY8);
	pdfr_encoder_set_compression(enc, PDFRAS_JPEG);
	// write a strip of raster data to the current page
	pdfr_encoder_write_strip(enc, 1100, gray_page_jpg, sizeof gray_page_jpg);
	// the page is done
	pdfr_encoder_end_page(enc);
}

int write_gray8_jpeg_file(t_OS os, const char *filename, int devColor)
{
	// Write a file: 4" x 5.5" at 2.0 DPI, uncompressed 8-bit grayscale
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	//pdfr_encoder_set_subject(enc, "GRAY8 JPEG sample output");
	pdfr_encoder_set_device_colorspace(enc, devColor);

	time_t tcd;
	pdfr_encoder_get_creation_date(enc, &tcd);
	set_xmp_create_date(XMP_metadata, tcd);
	pdfr_encoder_write_document_xmp(enc, XMP_metadata);

	write_gray8_jpeg_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_gray16_uncomp_page(t_pdfrasencoder* enc)
{
	// 16-bit grayscale!
	pdfr_encoder_set_resolution(enc, 16.0, 128.0);
	pdfr_encoder_start_page(enc, 64);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_GRAY16);
	pdfr_encoder_set_compression(enc, PDFRAS_UNCOMPRESSED);
	pdfr_encoder_set_physical_page_number(enc, 2);			// physical page 2
	// write a strip of raster data to the current page
	pdfr_encoder_write_strip(enc, 512, (const pduint8*)deepGrayData, sizeof deepGrayData);
	pdfr_encoder_end_page(enc);
}

int write_gray16_uncompressed_file(t_OS os, const char *filename)
{
	// Write a file: 4" x 5.5" at 2.0 DPI, uncompressed 8-bit grayscale
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_subject(enc, "GRAY16 Uncompressed sample output");

	write_gray16_uncomp_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_rgb24_uncomp_page(t_pdfrasencoder* enc)
{
	pdfr_encoder_set_resolution(enc, 50.0, 50.0);
	pdfr_encoder_set_rotation(enc, 90);
	pdfr_encoder_start_page(enc, 175);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_RGB24);
	pdfr_encoder_set_compression(enc, PDFRAS_UNCOMPRESSED);
	pdfr_encoder_write_strip(enc, 100, (pduint8*)colorData, sizeof colorData);
	pdfr_encoder_end_page(enc);
}

int write_rgb24_uncompressed_file(t_OS os, const char* filename)
{
	// Write a file: 24-bit RGB color 3.5" x 2" 50 DPI
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_subject(enc, "RGB24 Uncompressed sample output");

	write_rgb24_uncomp_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_rgb24_uncomp_multistrip_page(t_pdfrasencoder* enc)
{
	int stripheight = 20, r;
	pduint8* data = (pduint8*)colorData;
	size_t stripsize = 175 * 3 * stripheight;

	pdfr_encoder_set_resolution(enc, 50.0, 50.0);
	pdfr_encoder_set_rotation(enc, 0);
	pdfr_encoder_start_page(enc, 175);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_RGB24);
	pdfr_encoder_set_compression(enc, PDFRAS_UNCOMPRESSED);
	for (r = 0; r < 100; r += stripheight) {
		pdfr_encoder_write_strip(enc, stripheight, data, stripsize);
		data += stripsize;
	}
	if (pdfr_encoder_get_page_height(enc) != 100) {
		fprintf(stderr, "wrong page height at end of write_rgb24_uncomp_multistrip_page");
		exit(1);
	}
	pdfr_encoder_end_page(enc);
}

int write_rgb24_uncompressed_multistrip_file(t_OS os, const char* filename)
{
	// Write a file: 24-bit RGB color 3.5" x 2" 50 DPI, multiple strips.
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_subject(enc, "RGB24 Uncompressed multi-strip sample output");

	write_rgb24_uncomp_multistrip_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

void write_rgb24_jpeg_page(t_pdfrasencoder* enc)
{
	pdfr_encoder_set_resolution(enc, 100.0, 100.0);
	pdfr_encoder_set_rotation(enc, -180);
	pdfr_encoder_start_page(enc, 850);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_RGB24);
	pdfr_encoder_set_compression(enc, PDFRAS_JPEG);
	pdfr_encoder_write_strip(enc, 1100, color_page_jpg, sizeof color_page_jpg);
	pdfr_encoder_end_page(enc);
}


int write_rgb24_jpeg_file(t_OS os, const char *filename)
{
	// Write a file: JPEG-compressed color US letter page (stored upside-down)
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_title(enc, filename);
	pdfr_encoder_set_subject(enc, "24-bit JPEG-compressed sample output");

	pdfr_encoder_write_document_xmp(enc, XMP_metadata);

	write_rgb24_jpeg_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}

int write_allformat_multipage_file(t_OS os, const char *filename)
{
	// Write a multipage file containing all the supported pixel formats
	//
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	// Construct a raster PDF encoder
	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");

	pdfr_encoder_write_document_xmp(enc, XMP_metadata);

	pdfr_encoder_set_physical_page_number(enc, 1);
	pdfr_encoder_set_page_front(enc, 1);					// front side
	write_bitonal_uncomp_page(enc);

	pdfr_encoder_set_physical_page_number(enc, 1);
	pdfr_encoder_set_page_front(enc, 0);					// back side
	write_bitonal_ccitt_page(enc);

	pdfr_encoder_set_physical_page_number(enc, 2);
	write_gray8_uncomp_page(enc);

	pdfr_encoder_set_physical_page_number(enc, 3);
	write_gray8_jpeg_page(enc);

	pdfr_encoder_set_physical_page_number(enc, 4);
	write_gray16_uncomp_page(enc);

	pdfr_encoder_set_physical_page_number(enc, 5);
	write_rgb24_jpeg_page(enc);

	pdfr_encoder_set_physical_page_number(enc, 6);
	write_rgb24_uncomp_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);

	return 0;
}

void write_rgb24_jpeg_multistrip_page(t_pdfrasencoder* enc)
{

	pdfr_encoder_set_resolution(enc, 100.0, 100.0);
	pdfr_encoder_set_rotation(enc, 0);
	pdfr_encoder_start_page(enc, 850);
	pdfr_encoder_set_pixelformat(enc, PDFRAS_RGB24);
	pdfr_encoder_set_compression(enc, PDFRAS_JPEG);
	for (int stripno = 0; ; stripno++) {
		char filename[64];
		sprintf_s(filename, 64, "color_strip%d.jpg", stripno);
		FILE *fstrip = fopen(filename, "rb");
		if (!fstrip) {
			break;
		}
		static pduint8 data[1000000];
		size_t stripsize = fread(data, 1, sizeof data, fstrip);
		fclose(fstrip);
		pdfr_encoder_write_strip(enc, 275, data, stripsize);
	}
	if (pdfr_encoder_get_page_height(enc) != 1100) {
		fprintf(stderr, "wrong page height at end of write_rgb24_jpeg_multistrip_page");
		exit(1);
	}
	pdfr_encoder_end_page(enc);
}

int write_rgb24_jpeg_multistrip_file(t_OS os, const char* filename)
{
	// Write a file: 24-bit RGB color 8.5x11" page in three JPEG strips
	FILE *fp = fopen(filename, "wb");
	if (fp == 0) {
		fprintf(stderr, "unable to open %s for writing\n", filename);
		return 1;
	}
	os.writeoutcookie = fp;
	os.allocsys = pd_alloc_sys_new(&os);

	t_pdfrasencoder* enc = pdfr_encoder_create(PDFRAS_API_LEVEL, &os);
	pdfr_encoder_set_creator(enc, "raster_encoder_demo 1.0");
	pdfr_encoder_set_subject(enc, "RGB24 JPEG multi-strip sample output");

	write_rgb24_jpeg_multistrip_page(enc);

	// the document is complete
	pdfr_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pdfr_encoder_destroy(enc);
	return 0;
}


int main(int argc, char** argv)
{
	t_OS os;
	os.alloc = mymalloc;
	os.free = free;
	os.memset = myMemSet;
	os.writeout = myOutputWriter;

	generate_image_data();

	//write_rgb24_jpeg_multistrip_file(os, "sample rgb24 jpeg multistrip.pdf");

	write_bitonal_uncompressed_file(os, "sample bw1 uncompressed.pdf");

	write_bitonal_ccitt_file(os, "sample bw1 ccitt.pdf");

	write_gray8_uncompressed_file(os, "sample gray8 uncompressed.pdf");

	write_gray8_jpeg_file(os, "sample gray8-calibrated jpeg.pdf", 0);

	write_gray8_jpeg_file(os, "sample gray8-device jpeg.pdf", 1);

	write_gray16_uncompressed_file(os, "sample gray16 uncompressed.pdf");

	write_rgb24_uncompressed_file(os, "sample rgb24 uncompressed.pdf");

	write_rgb24_uncompressed_multistrip_file(os, "sample rgb24 uncompressed multistrip.pdf");

	write_rgb24_jpeg_file(os, "sample rgb24 jpeg.pdf");

	write_allformat_multipage_file(os, "sample all formats.pdf");

	return 0;
}

