
# This is a makefile for building on non-Windows platforms.

H =	../pdfras_writer/PdfRaster.h
A = ../pdfras_writer/libpdfras_writer.a

CFLAGS = -O -g -I"../common" -I"../pdfras_writer" -lpdfras_writer

LDFLAGS = -L../pdfras_writer

LIBS = -lpdfras_writer

pdfras_writer_tests: pdfras_writer_tests.c ../common/test_support.c

clean:
	rm -rf *.dSYM
	rm -f *. *.o
