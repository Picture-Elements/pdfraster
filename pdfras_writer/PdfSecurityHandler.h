#ifndef _H_PdfSecurityHandler
#define _H_PdfSecurityHandler
#pragma once

#include "PdfOS.h"

typedef struct t_pdencrypter t_pdencrypter;

// functions associated with an (encryption) security handler:
// * Creating an encryption state for a PDF from whatever it needs as input
// * Setting up to encrypt one object, using state+(onr,gen).
// * Predicting the buffer size needed to encrypt n bytes of data
// * Encrypting n bytes of data
// * Writing (or providing) all the encryption metadata

// initialize for encryption of an object.
// The object, or it's first indirect parent, is indirect object <onr, genr>.
extern void pd_encrypt_start_object(t_pdencrypter *crypter, pduint32 onr, pduint32 genr);

// calculate the encrypted size of n bytes of plain data
extern pduint32 pd_encrypted_size(t_pdencrypter *crypter, pduint32 n);

// encrypt n bytes of data
extern void pd_encrypt_data(t_pdencrypter *crypter, pduint8 *outbuf, const pduint8* data, pduint32 n);

#endif
