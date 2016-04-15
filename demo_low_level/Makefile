
# This is a makefile for building on non-Windows platforms.

H =	../common/portability.h \
    ../pdfras_writer/PdfOS.h \
    ../pdfras_writer/PdfAlloc.h \
    ../pdfras_writer/PdfDict.h \
    ../pdfras_writer/PdfStreaming.h \
    ../pdfras_writer/PdfAtoms.h \
    ../pdfras_writer/PdfStandardAtoms.h \
    ../pdfras_writer/PdfXrefTable.h \
    ../pdfras_writer/PdfImage.h \
    ../pdfras_writer/PdfStandardObjects.h \
    ../pdfras_writer/PdfContentsGenerator.h

CFLAGS = -O -g -I"../common" -I"../pdfras_writer" -lpdfras_writer

LDFLAGS = -L../pdfras_writer

LIBS = -lpdfras_writer

demo_low_level: demo_low_level.c

clean:
	rm -rf *.dSYM
	rm -f *.o
	rm -f demo_low_level
