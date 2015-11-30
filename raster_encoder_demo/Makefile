
WRITER = ../pdfras_writer

O = raster_encoder_demo.o
A = $(WRITER)/libpdfras_writer.a

CFLAGS = -O -g -I$(WRITER)
CC = gcc -std=gnu99

raster_encoder_demo: $O $A
	$(CC) -o raster_encoder_demo $O $A -lm

raster_encoder_demo.o: raster_encoder_demo.c $(WRITER)/PdfRaster.h bw_ccitt_data.h color_page.h

bw_ccitt_page.h: bw_ccitt_page.bin bin2carray.exe
	./bin2carray.exe -i bw_ccitt_page.bin -s bw_ccitt_data > bw_ccitt_data.h

color_page.h: color_page.jpg bin2carray.exe
	./bin2carray.exe -i color_page.jpg -s color_page_jpg > color_page.h

bin2carray.exe: bin2carray.c
	$(CC) -o bin2carray.exe bin2carray.c
