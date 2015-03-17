// raster_encoder_demo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "PdfRaster.h"

#include "bw_ccitt_data.h"
#include "color_page.h"

#define OUTPUT_FILENAME "raster.pdf"

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

int main(int argc, char** argv)
{
	FILE *fp;
	t_OS os;

	if (fopen_s(&fp, OUTPUT_FILENAME, "wb") != 0) {
		fprintf(stderr, "unable to open %s for writing\n", OUTPUT_FILENAME);
		return 1;
	}
	os.alloc = mymalloc;
	os.free = free;
	os.memset = myMemSet;
	os.memclear = myMemClear;
	os.allocsys = pd_alloc_sys_new(&os);
	os.writeout = myOutputWriter;
	os.writeoutcookie = fp;

	// Construct a raster PDF encoder
	t_pdfrasencoder* enc = pd_raster_encoder_create(PDFRAS_API_LEVEL, &os);
	pd_raster_set_creator(enc, "raster_encoder_demo 1.0");

	// First page - 4" x 5.5" at 2 DPI
	pd_raster_set_resolution(enc, 2.0, 2.0);
	// start a new page
	pd_raster_encoder_start_page(enc, PDFRAS_GRAY8, PDFRAS_UNCOMPRESSED, 8);
	pd_raster_set_physical_page_number(enc, 1);			// physical page 1
	pd_raster_set_page_front(enc, 1);					// front side
	// write a strip of raster data to the current page
	// 11 rows high
	pd_raster_encoder_write_strip(enc, 11, _imdata, sizeof _imdata);
	// the page is done
	pd_raster_encoder_end_page(enc);

	// Next page - 16-bit grayscale!
	// generate the data - 64 columns, 512 rows
	for (int i = 0; i < 64*512; i++) {
		int y = (i / 64);
		unsigned value = 65535 - (y * 65535 / 511);
		pduint8* pb = (pduint8*)(deepGrayData + i);
		pb[0] = value / 256;
		pb[1] = value % 256;
	}
	pd_raster_set_resolution(enc, 16.0, 128.0);
	pd_raster_encoder_start_page(enc, PDFRAS_GRAY16, PDFRAS_UNCOMPRESSED, 64);
	pd_raster_set_physical_page_number(enc, 2);			// physical page 2
	// write a strip of raster data to the current page
	pd_raster_encoder_write_strip(enc, 512, (const pduint8*)deepGrayData, sizeof deepGrayData);
	pd_raster_encoder_end_page(enc);

	// Next page: bitonal 8.5 x 11 at 100 DPI with a light dotted grid
	// generate page data
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
	pd_raster_set_resolution(enc, 100.0, 100.0);
	pd_raster_encoder_start_page(enc, PDFRAS_BITONAL, PDFRAS_UNCOMPRESSED, 850);
	pd_raster_encoder_write_strip(enc, 1100, bitonalData, sizeof bitonalData);
	pd_raster_set_physical_page_number(enc, 3);			// physical page 2
	pd_raster_set_page_front(enc, 1);					// front side
	pd_raster_encoder_end_page(enc);

	// Next page: CCITT-compressed B&W 300 DPI US Letter (scanned)
	pd_raster_set_resolution(enc, 300.0, 300.0);
	pd_raster_encoder_start_page(enc, PDFRAS_BITONAL, PDFRAS_CCITTG4, 2521);
	pd_raster_set_physical_page_number(enc, 3);			// physical page 2
	pd_raster_set_page_front(enc, 0);					// back side
	pd_raster_encoder_write_strip(enc, 3279, bw_ccitt_data, sizeof bw_ccitt_data);
	pd_raster_encoder_end_page(enc);

	// Next page: color 3.5" x 2" 50 DPI
	struct { unsigned char R, G, B;  } colorData[175*100];
	for (int i = 0; i < (175*100); i++) {
		int y = (i / 175);
		int x = (i % 175);
		colorData[i].R = x * 255 / 175;
		colorData[i].G = y * 255 / 100;
		colorData[i].B = (x+y) * 255 / (100+175);
	}
	pd_raster_set_resolution(enc, 50.0, 50.0);
	pd_raster_set_rotation(enc, 90);
	pd_raster_encoder_start_page(enc, PDFRAS_RGB24, PDFRAS_UNCOMPRESSED, 175);
	pd_raster_encoder_write_strip(enc, 100, (pduint8*)colorData, sizeof colorData);
	pd_raster_encoder_end_page(enc);

	// Next page: JPEG-compressed color US letter page (stored upside-down)
	pd_raster_set_resolution(enc, 100.0, 100.0);
	pd_raster_set_rotation(enc, -180);
	pd_raster_encoder_start_page(enc, PDFRAS_RGB24, PDFRAS_JPEG, 850);
	pd_raster_encoder_write_strip(enc, 1100, color_page_jpg, sizeof color_page_jpg);
	pd_raster_encoder_end_page(enc);

	// the document is complete
	pd_raster_encoder_end_document(enc);
	// clean up
	fclose(fp);
	pd_raster_encoder_destroy(enc);

	return 0;
}

