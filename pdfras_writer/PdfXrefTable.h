#ifndef _H_PdfXrefTable
#define _H_PdfXrefTable

#include "PdfValues.h"
#include "PdfAlloc.h"
#include "PdfStreaming.h"

///////////////////////////////////////////////////////////////////////
// References (more or less, Indirect Objects)

// Return the object number of an indirect object.
extern pduint32 pd_reference_object_number(t_pdvalue ref);

// Return PD_TRUE if an indirect object (technically, its definition)
// has been written to an output stream.
extern pdbool pd_reference_is_written(t_pdvalue ref);

// Mark an indirect object has having been written.
extern void pd_reference_mark_written(t_pdvalue ref);

// Get the value of an indirect object.
// Is null until its value has been set.
extern t_pdvalue pd_reference_get_value(t_pdvalue ref);

// Set the value of an unresolved/forward indirect object.
// ref is a reference to the (unresolved) indirect object.
// value is its now-known value.
// You can only resolve an unresolved object once.
// (OK, you could repeatedly resolve it to null.)
extern void pd_reference_resolve(t_pdvalue ref, t_pdvalue value);

// Get the file position of (the definition of) an indirect object.
extern pduint32 pd_reference_get_position(t_pdvalue ref);

// Record the file position of (the definition of) an indirect object.
extern void pd_reference_set_position(t_pdvalue ref, pduint32 pos);

///////////////////////////////////////////////////////////////////////
// XREF tables

// Create a new empty XREF table:
extern t_pdxref *pd_xref_new(t_pdmempool *pool);

// Release storage allocated to an XREF table.
extern void pd_xref_free(t_pdxref *xref);

// Create an indirect object of given value.
// xref is the XREF table to register the object in.
// value is ... the value of the indirect object.
// If there is already an indirect object in the table that
// is guaranteed to be the same (meaning, same dictionary or array)
// this function will return a reference to that previously defined object.
extern t_pdvalue pd_xref_makereference(t_pdxref *xref, t_pdvalue value);

// Create a unique indirect object and register in XREF table.
// xref is the XREF table to register the object in.
// The created object is different from any previously created
// indirect object (in that xref table) and has default value = null.
extern t_pdvalue pd_xref_create_forward_reference(t_pdxref *xref);

// The number of entries in the XREF table.
extern pdint32 pd_xref_size(t_pdxref *xref);

// Write definitions out to stream, for any objects referenced in XREF table but not yet defined.
extern void pd_xref_writeallpendingreferences(t_pdxref *xref, t_pdoutstream *os);

// Write an XREF table out to a stream.
// Does not check for or write out not-yet-defined objects. 
extern void pd_xref_writetable(t_pdxref *xref, t_pdoutstream *os);

#endif
