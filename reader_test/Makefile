
# This is a makefile for building on non-Windows platforms.

H =	../pdfras_writer/PdfRaster.h
A = ../pdfras_reader/libpdfras_reader.a

CPPFLAGS = -O -g -I"../common" -I"../pdfras_reader" -I"../pdfras_writer" -lpdfras_reader

LDFLAGS = -L../pdfras_reader

LIBS = -lpdfras_reader

reader_test: reader_test.c ../common/test_support.c

clean:
	rm -rf *.dSYM *.o reader_test
