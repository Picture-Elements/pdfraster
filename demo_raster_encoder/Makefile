
WRITER = ../pdfras_writer

O = demo_raster_encoder.o
A = $(WRITER)/libpdfras_writer.a

CFLAGS = -O -g -I$(WRITER)
CC = gcc -std=gnu99

demo_raster_encoder: $O $A
	$(CC) -o demo_raster_encoder $O $A -lm

demo_raster_encoder.o: demo_raster_encoder.c $(WRITER)/PdfRaster.h bw_ccitt_data.h color_page.h

bw_ccitt_page.h: bw_ccitt_page.bin bin2carray.exe
	./bin2carray.exe -i bw_ccitt_page.bin -s bw_ccitt_data > bw_ccitt_data.h

color_page.h: color_page.jpg bin2carray.exe
	./bin2carray.exe -i color_page.jpg -s color_page_jpg > color_page.h

bin2carray.exe: bin2carray.c
	$(CC) -o bin2carray.exe bin2carray.c
