
# This is a makefile for building on non-Windows platforms.

O =	PdfAlloc.o \
	PdfArray.o \
	PdfAtoms.o \
	PdfContentsGenerator.o \
	PdfDatasink.o \
	PdfDict.o \
	PdfElements.o \
	PdfHash.o \
	PdfOS.o \
	PdfRaster.o \
	PdfStandardObjects.o \
	PdfStreaming.o \
	PdfString.o \
	PdfStrings.o \
	PdfXrefTable.o

CFLAGS = -O -g

libpdfras_writer.a: $O
	@rm -f $@
	ar cqv libpdfras_writer.a $O

PdfAlloc.o: PdfAlloc.c  PdfAlloc.h PdfPlatform.h
PdfArray.o: PdfArray.c  PdfArray.h PdfPlatform.h
PdfAtoms.o: PdfAtoms.c  PdfAtoms.h PdfStandardAtoms.h PdfPlatform.h
PdfContentsGenerator.o: PdfContentsGenerator.c PdfContentsGenerator.h PdfDatasink.h PdfStreaming.h PdfAlloc.h
PdfDatasink.o: PdfDatasink.c PdfDatasink.h PdfElements.h PdfAlloc.h
PdfDict.o: PdfDict.c PdfDict.h PdfHash.h PdfElements.h PdfAtoms.h PdfDatasink.h PdfXrefTable.h PdfStandardAtoms.h
PdfElements.o: PdfElements.c PdfElements.h PdfString.h PdfStrings.h
PdfHash.o: PdfHash.c PdfHash.h PdfStandardAtoms.h PdfStrings.h
PdfOS.o: PdfOS.c PdfOS.h PdfPlatform.h
PdfRaster.o: PdfRaster.c PdfRaster.h PdfDict.h PdfAtoms.h PdfStandardAtoms.h PdfString.h PdfXrefTable.h PdfStandardObjects.h PdfArray.h
PdfStandardObjects.o: PdfStandardObjects.c PdfStandardObjects.h PdfStrings.h PdfStandardAtoms.h PdfDict.h PdfArray.h PdfContentsGenerator.h
PdfStreaming.o: PdfStreaming.c PdfStreaming.h PdfDict.h PdfAtoms.h PdfString.h PdfXrefTable.h PdfStandardObjects.h PdfArray.h
PdfString.o: PdfString.c PdfString.h
PdfStrings.o: PdfStrings.c PdfStrings.h
PdfXrefTable.o: PdfXrefTable.c PdfXrefTable.h
