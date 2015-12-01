#ifndef _H_PdfContentsGenerator
#define _H_PdfContentsGenerator
#pragma once

#include "PdfAlloc.h"
#include "PdfDatasink.h"
typedef struct t_pdcontents_gen t_pdcontents_gen;
typedef void (*f_gen)(t_pdcontents_gen *gen, void *gencookie);

extern t_pdcontents_gen *pd_contents_gen_new(t_pdallocsys *alloc, f_gen gen, void *gencookie);
extern void pd_contents_gen_free(t_pdcontents_gen *gen);

extern void pd_contents_generate(t_datasink *sink, void *eventcookie);

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
