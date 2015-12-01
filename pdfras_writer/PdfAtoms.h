#ifndef _H_PdfAtoms
#define _H_PdfAtoms
#pragma once

#include "PdfOS.h"
#include "PdfValues.h"

typedef struct t_pdatomtable t_pdatomtable;

extern const char *pd_atom_name(t_pdatom atom);

extern t_pdatom pd_atom_intern(t_pdatomtable *atoms, const char* name);

extern t_pdatomtable* pd_atom_table_new(t_pdallocsys* pool, int initialCap);

// Free an atom table, including all interned atoms.
extern void pd_atom_table_free(t_pdatomtable* atoms);

// Return the number of distinct entries (atoms) in an atom table.
extern int pd_atom_table_count(t_pdatomtable* atoms);

#endif
