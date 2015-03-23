
WRITER = ../pdfras_writer

O = raster_encoder_demo.o
A = $(WRITER)/libpdfras_writer.a

CFLAGS = -O -g -I$(WRITER)
CC = gcc -std=gnu99

raster_encoder_demo: $O $A
	$(CC) -o raster_encoder_demo $O $A -lm

raster_encoder_demo.o: raster_encoder_demo.c $(WRITER)/PdfRaster.h bw_ccitt_data.h color_page.h
