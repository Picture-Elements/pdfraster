
# This is a makefile for building on non-Windows platforms.

H =	../pdfras_writer/PdfRaster.h
A = ../pdfras_writer/libpdfras_writer.a

CFLAGS = -O -g -I../pdfras_writer -I../common

pdfras_writer_tests: pdfras_writer_tests.c ../common/test_support.c $(A)

pdfras_writer_tests.o: pdfras_writer_tests.c $(H)
