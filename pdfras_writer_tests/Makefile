
# This is a makefile for building on non-Windows platforms.

H =	../pdfras_writer/PdfRaster.h
A = ../pdfras_writer/libpdfras_writer.a

CFLAGS = -O -g -I../pdfras_writer

pdfras_writer_tests: pdfras_writer_tests.o $(A)

pdfras_writer_tests.o: pdfras_writer_tests.c $(H)
