// raster_encoder_demo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// function from miniz.c to deflate the ICC profile
extern size_t tdefl_compress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

const char* profile_data_file = "sRGB_IEC61966-2-1_black_scaled.icc";
const char* output_include_file = "srgb_icc_profile.h";

// write nb bytes of data to out, formatted as C-style byte array initialization
// in 16 columns, like so:
//  ...
//  0x37, 0xf6, 0xad, 0x11, 0x0a, 0x83, 0x64, 0xdf, 0x52, 0x29, 0xd9, 0x53, 0x19, 0x33, 0x83, 0xb1,
//  ...
static void format_data(FILE* out, unsigned char *data, size_t nb)
{
	size_t i;
	for (i = 0; i < nb; i++) {
		fprintf(out, " 0x%02x", data[i]);
		if (i + 1 == nb) break;
		fputc(',', out);
		if ((i % 16) == 15) {
			fputc('\n', out);
		}
	}
	fputc('\n', out);
}

int main(int argc, char** argv)
{
	FILE* in = fopen(profile_data_file, "rb");
	if (!in) {
		fprintf(stderr, "unable to open .icc input file\n");
		return 1;
	}
	FILE* out = fopen(output_include_file, "w");
	if (!out) {
		fprintf(stderr, "unable to create output file: %s\n", output_include_file);
	}
	else {
		unsigned char data[65536];
		fprintf(out, "// data from file: %s\n", profile_data_file);
		size_t nb = fread(data, 1, sizeof data, in);
		//// deflate memory-to-memory with max compression
		// Deflate drops the profile size from ~3000 bytes to ~2500 bytes. Hardly seems worth it.
		//unsigned char zdata[65536];
		//nb = tdefl_compress_mem_to_mem(zdata, sizeof zdata, data, nb, 0xFFF);
		fputs("static pduint8 srgb_icc_profile[] = {\n", out);
		format_data(out, data, nb);
		fputs("};\n", out);
		fclose(out);
	}
	fclose(in);
}
