#ifndef _H_pdfras_files
#define _H_pdfras_files
#pragma once

#include "pdfrasread.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return TRUE if the file starts with the signature of a PDF/raster file.
// FALSE otherwise.
int pdfrasread_recognize_file(FILE* f);

// Return TRUE if the file starts with the signature of a PDF/raster file.
// FALSE otherwise.
int pdfrasread_recognize_filename(const char* fn);

int pdfrasread_page_count_file(FILE* f);
int pdfrasread_page_count_filename(const char* fn);

// create a PDF/raster reader and use it to access a FILE
t_pdfrasreader* pdfrasread_open_file(int apiLevel, FILE* f);

// create a PDF/raster reader and use it to open a named file
t_pdfrasreader* pdfrasread_open_filename(int apiLevel, const char* fn);

#ifdef __cplusplus
}
#endif
#endif
