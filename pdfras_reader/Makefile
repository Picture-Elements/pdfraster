
# This is a makefile for building on non-Windows platforms.

STATICLIB= libpdfras_reader.a

O =	pdfrasread_files.o \
    pdfrasread.o

CFLAGS = -O -g -I"../pdfras_writer"

# create a static library for pdfras_reader
$(STATICLIB): $O
	rm -f $@
	ar rcs $(STATICLIB) $O

# compile all the individual object modules
pdfrasread.o: pdfrasread.c

pdfrasread_files.o: pdfrasread_files.c

clean:
	rm -rf *.a *.o
