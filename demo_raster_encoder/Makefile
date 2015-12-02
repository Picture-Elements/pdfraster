
WRITER = ../pdfras_writer

O = demo_raster_encoder.o
A = $(WRITER)/libpdfras_writer.a

CFLAGS = -O -g -I$(WRITER)
CC = gcc -std=gnu99

demo_raster_encoder: $O $A
	$(CC) -o demo_raster_encoder $O $A -lm

demo_raster_encoder.o: demo_raster_encoder.c $(WRITER)/PdfRaster.h bw_ccitt_data.h color_page.h gray8_page.h \
	color_strip0.h color_strip1.h color_strip2.h color_strip3.h

bw_ccitt_page.h: bw_ccitt_page.bin bin2carray.exe
	./bin2carray.exe -i bw_ccitt_page.bin -s bw_ccitt_page_bin > bw_ccitt_data.h

color_page.h: color_page.jpg bin2carray.exe
	./bin2carray.exe -i color_page.jpg -s color_page_jpg > color_page.h

gray8_page.h: gray8_page.jpg bin2carray.exe
	./bin2carray.exe -i gray8_page.jpg -s gray8_page_jpg > gray8_page.h

color_strip0.h: color_strip0.jpg bin2carray.exe
	./bin2carray.exe -i color_strip0.jpg -s color_strip0_jpg > color_strip0.h

color_strip1.h: color_strip1.jpg bin2carray.exe
	./bin2carray.exe -i color_strip1.jpg -s color_strip1_jpg > color_strip1.h

color_strip2.h: color_strip2.jpg bin2carray.exe
	./bin2carray.exe -i color_strip2.jpg -s color_strip2_jpg > color_strip2.h

color_strip3.h: color_strip3.jpg bin2carray.exe
	./bin2carray.exe -i color_strip3.jpg -s color_strip3_jpg > color_strip3.h

bin2carray.exe: bin2carray.c
	$(CC) -o bin2carray.exe bin2carray.c
