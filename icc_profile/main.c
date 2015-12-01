// raster_encoder_demo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	FILE* in = fopen("sRGB_IEC61966-2-1_black_scaled.icc", "rb");
	if (!in) {
		fprintf(stderr, "unable to open .icc input file\n");
		return 1;
	}
	FILE* out = fopen("srgb_icc_profile.h", "w");
	if (!out) {
		fprintf(stderr, "unable to create srgb_icc_profile.h output file\n");
	}
	else {
		unsigned char data[65536];
		unsigned i;
		fputs("// sRGB ICC profile\n", out);
		size_t nb = fread(data, 1, sizeof data, in);
		for (i = 0; i < nb; i++) {
			fprintf(out, " 0x%02x,", data[i]);
			if ((i % 16) == 15) fputc('\n', out);
		}
		fputc('\n', out);
		fclose(out);
	}
	fclose(in);
}
