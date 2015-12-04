#ifndef _H_PdfContentsGenerator
#define _H_PdfContentsGenerator
#pragma once

#include "PdfAlloc.h"
#include "PdfDatasink.h"

typedef struct t_pdcontents_gen t_pdcontents_gen;

// A content-generating function
typedef void (*f_gen)(t_pdcontents_gen *gen, void *gencookie);

// Create a new contents generator.
// (which consists of a content-generating function and a cookie.)
extern t_pdcontents_gen *pd_contents_gen_new(t_pdmempool *pool, f_gen gen, void *gencookie);

// Free a contents generator
extern void pd_contents_gen_free(t_pdcontents_gen *gen);

// Activate a contents-generator to output its contents to a datasink.
extern void pd_contents_generate(t_datasink *sink, void *gen);

// Helper functions for generating PDF content stream operators.
extern void pd_gen_moveto(t_pdcontents_gen *gen, pddouble x, pddouble y);
extern void pd_gen_lineto(t_pdcontents_gen *gen, pddouble x, pddouble y);
extern void pd_gen_closepath(t_pdcontents_gen *gen);
extern void pd_gen_stroke(t_pdcontents_gen *gen);
extern void pd_gen_fill(t_pdcontents_gen *gen, pdbool evenodd);
extern void pd_gen_gsave(t_pdcontents_gen *gen);
extern void pd_gen_grestore(t_pdcontents_gen *gen);
extern void pd_gen_concatmatrix(t_pdcontents_gen *gen, pddouble a, pddouble b, pddouble c, pddouble d, pddouble e, pddouble f);
extern void pd_gen_xobject(t_pdcontents_gen *gen, t_pdatom xobjectatom);

#endif
