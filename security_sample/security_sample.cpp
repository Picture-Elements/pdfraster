
// May 2016 Roman Toda (roman.toda@gmail.com)
//
// This sample file is prepared for purposes of PDF/R working gropup (www.pdfa.org)
// to demonstrate how OpenSSL can be used to digitally sign pdf documents and to decrypt
// pdf files secured with Standard security handlers.
//
// Feel free to distribute or change just make sure you understand that this is just 
// to show basic principles. A lot os steps are missing, some algorithms are skiped
// and we dn't care here about good programming principles like error handling or memory leaks ;) 

#include "stdafx.h"

#include "openssl/crypto.h"
#include "openssl/pem.h"
#include "openssl/pkcs12.h"
#include "openssl/err.h"
#include "openssl/md5.h"

// Some helper functions here
int copy_file(const wchar_t* in_file_name, const wchar_t* out_file_name) 
{
  FILE *in_file, *out_file;

  in_file = _wfopen(in_file_name, L"rb");
  if (in_file == NULL)
    return 0;
  out_file = _wfopen(out_file_name, L"wb");
  if (out_file == NULL)
    return 0;

  char buffer[1024];
  size_t bytes;
  while (0 < (bytes = fread(buffer, 1, sizeof(buffer), in_file)))
    fwrite(buffer, 1, bytes, out_file);

  fclose(in_file);
  fclose(out_file);
  return 1;
}

void read_string_from_file(const wchar_t* in_file_name, int offset, int len, unsigned char *out_buffer, int *out_len)
{
  *out_len = 0;
  FILE *in_file = _wfopen(in_file_name, L"rb");
  if (in_file == NULL)
    return;

  fseek(in_file, offset, SEEK_SET);
  *out_len = fread(out_buffer, 1, len, in_file);
  fclose(in_file);
  return;
}



// ******************* Digital Signature 
// We have pfx file woith certificate, we have prepared pdf file
// and we need just used OpenSSL to sign two buffers and then to put the 
// signature into the file at specific location

// pdf processor did a lot of work before we can sign.
// it prepared several objects in the pdf file 
// new annotation, that was inserted into annotation array 
// new AcroForm field that points to the same annotation
// the annotation points to following dictionary 
// where the processor left empty space for signature 
// as well as ByteRange that idenfies two buffers to be signed

// here is important dictionary that we have hardcoded here
// ByteRange defines two buffer we need to sign and zeros in the \Contens is 
//just empty space where we will put our signature

//33 0 obj
//<<
//  / ByteRange[0 27783 30321 527] 
//  / Contents<000.......000> 
//  / Type / Sig 
//  / Filter / Adobe.PPKLite 
//  / SubFilter / adbe.pkcs7.detached 
//  / Reason(Testing OpenSSL for PDF / R) 
//  / Location(Hill Valley) 
//  / Name(Roman Toda) 
//  / ContactInfo(roman.toda@gmail.com) 
//  / M(D:20160428133243 + 02'00')
//>>
//endobj


//******************* Hardcoded stuff is here:
//download testing certificate from: https://drive.google.com/file/d/0B1ppJrJ4SN8EZG1rY1RidVNvMjA/view?usp=sharing
#define PFX_FILE_NAME L"p:\\security\\RomanToda(test).pfx"
#define PFX_PASSWORD "openpassword"

//download prepared file to be signed from: https://drive.google.com/file/d/0B1ppJrJ4SN8ESEN6cklCZkh5ZkU/view?usp=sharing 
#define TO_SIGN_PDF_FILE L"p:\\security\\to_sign.pdf"

// hardcoded byterange from the pdf file
#define BYTERANGE_OFFSET_1 0
#define BYTERANGE_LEN_1 27783
#define BYTERANGE_OFFSET_2 30321
#define BYTERANGE_LEN_2 527

// Where to store signed document
#define SIGNED_PDF_FILE L"p:\\security\\signed.pdf"

int sign_file() 
{
  EVP_PKEY* pkey = NULL;
  X509* cert = NULL;
  STACK_OF(X509)* ca = NULL;
  PKCS12 *pkcs12;
  FILE *pfx_file, *signed_file;

  // OpenSSL init
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();

  // acquiring data from PFX certificate file
  if (!(pfx_file = _wfopen(PFX_FILE_NAME, L"rb")))
    return 0;
  pkcs12 = d2i_PKCS12_fp(pfx_file, NULL);
  fclose(pfx_file);
  if (!pkcs12)
    return 0;
 
  if (!PKCS12_parse(pkcs12, PFX_PASSWORD, &pkey, &cert, &ca))
  {
    PKCS12_free(pkcs12);
    return 0;
  }
  PKCS12_free(pkcs12);

  // copy original file 
  if (!copy_file(TO_SIGN_PDF_FILE, SIGNED_PDF_FILE))
    return 0;

  signed_file = _wfopen(SIGNED_PDF_FILE, L"rb+");
  if (signed_file == NULL)
    return 0;

  // going to sign the file here - 

  // buffer for data that we need to sign
  unsigned char *buffer_to_sign = (unsigned char*)malloc(BYTERANGE_LEN_1+ BYTERANGE_LEN_2);

  // preparing buffer for signature 
  int signed_len_in_hex = BYTERANGE_OFFSET_2 - BYTERANGE_LEN_1 - 2;
  int signed_len_in_bytes = 0;

  // we will store signature here
  unsigned char *signed_data = (unsigned char*)malloc(signed_len_in_hex);

  //hex interpretation of signature. We must allocate space of ending \0
  unsigned char *signed_data_hex = (unsigned char*)malloc(signed_len_in_hex+1); 
  memset(signed_data_hex, 0, signed_len_in_hex+1);
  memset(signed_data, 0, signed_len_in_hex);

  // reading file (except /Contents) to single buffer
  // pdf defines 2 buffers from beginning of the file to the start 
  // of empty space and then the rest 
  fseek(signed_file, BYTERANGE_OFFSET_1, SEEK_SET);
  fread((unsigned char *)buffer_to_sign, 1, BYTERANGE_LEN_1, signed_file);
  fseek(signed_file, BYTERANGE_OFFSET_2, SEEK_SET);
  fread((unsigned char *)(buffer_to_sign + BYTERANGE_LEN_1), 1, BYTERANGE_LEN_2, signed_file);

  // signing buffer with certificate
  BIO* inputbio = BIO_new(BIO_s_mem());
  BIO_write(inputbio, buffer_to_sign, (int)(BYTERANGE_LEN_1 + BYTERANGE_LEN_2));
  PKCS7 *pkcs7;
  int flags = PKCS7_DETACHED | PKCS7_BINARY;
  pkcs7 = PKCS7_sign(cert, pkey, NULL, inputbio, flags);
  BIO_free(inputbio);

  // going to acquire encrypted data
  if (pkcs7) {
    BIO* outputbio = BIO_new(BIO_s_mem());
    i2d_PKCS7_bio(outputbio, pkcs7);
    BUF_MEM* mem = NULL;
    BIO_get_mem_ptr(outputbio, &mem);
    if (mem && mem->data && mem->length) {
      // mem->length is supposed to be half of signed_len_in_hex
      // because /Contents is written in hex (so 2 characters for each byte)
      signed_len_in_bytes = mem->length;
      memcpy(signed_data, mem->data, signed_len_in_bytes);
    }
    BIO_free(outputbio);
    PKCS7_free(pkcs7);

    // converting to hex
    for (int i = 0; i < signed_len_in_bytes; i++)
      sprintf((char*)&signed_data_hex[i * 2], "%02X", signed_data[i]);

    // writing directly to /Content entry
    fseek(signed_file, BYTERANGE_OFFSET_1 + BYTERANGE_LEN_1 + 1, SEEK_SET);
    fwrite(signed_data_hex, 1, signed_len_in_bytes * 2, signed_file);
  }

  free(buffer_to_sign);
  free(signed_data);
  free(signed_data_hex);
  fclose(signed_file);

  // Cleanup OpenSSL
  EVP_cleanup();
  return 1;
}


// ******************* Password security AES 256
// PDF spec clearly describes an algorithm how to get encryption key from entered password 
// it is different with every algorithm
// I skipped that part here and hardcoded encryption key here I got from open password ("open")
// Every string object you read from pdf file has to be preprocessed. You need to alter "\t"-> 0x9 
// for example as described in PDF Spec (TABLE 3.2)
//
// To ilustrate how to use OpenSSL to decrypt string I've picked string FontFanily from following dictionary
// for simple reason that this one doesn't contain any special character, so I can skip here the conversion

/*
23 0 obj
<< / FontBBox[-568 - 307 2046 1040]
/ FontFile2 16 0 R
/ FontName / DAHGOL + TimesNewRomanPSMT
/ Flags 6
/ ItalicAngle 0
/ Ascent 891
/ Descent - 216
/ CapHeight 1000
/ XHeight 1000
/ StemV 82
/ Type / FontDescriptor
/ CIDSet 15 0 R
/ FontFamily(o—ŒC˜g÷§PÚ&; }6 VaQ°J FÙe¹‡Ìá¢)
/ FontStretch / Normal
/ FontWeight 400
>>
*/

//******************* Hardcoded stuff is here:
//download testing file from: https://drive.google.com/open?id=0B1ppJrJ4SN8EZGFBcEthOFBmREE
#define FILE_NAME_256AES L"p:\\security\\secured 256AES (AAX).pdf"
static const unsigned char ENCRYPTION_KEY_256AES[32] = {
  0x99, 0x64, 0xA4, 0x14, 0x07, 0x29, 0x85, 0xA7, 0x3C, 0x8D, 0xCE, 0x5F, 0xBE, 0x13, 0x15, 0x45,
  0xD6, 0x24, 0x71, 0x0D, 0x36, 0x3A, 0x92, 0x2D, 0x55, 0xFE, 0xAD, 0xBF, 0x9F, 0x2C, 0xB1, 0x37
};
//static int ENCRYPTION_KEY_LENGTH_256AES = 32;
#define FONTFAMILY_OFFSET 22571
#define FONTFAMILY_LENGTH 32

int decrypt_file_256AES()
{
  unsigned char encrypted_buffer[1024];
  unsigned char decrypted_buffer[1024];
  int encrypted_len = FONTFAMILY_LENGTH;
  int decrypted_len;
  int len;

  // reading specific buffer from the pdf file
  read_string_from_file(FILE_NAME_256AES, FONTFAMILY_OFFSET, encrypted_len, encrypted_buffer, &decrypted_len);

  // OpenSSL init
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();

  //PDF spec says:
  //If using the AES algorithm, the Cipher Block Chaining (CBC) mode, which requires an initialization vector, is used. 
  //The block size parameter is set to 16 bytes, and the initialization vector is a 16-byte random number that is stored 
  //as the first 16 bytes of the encrypted stream or string.
  //
  // therefore when decrypting we set iv as encrypted buffer 
  // and when decryption we simply ignore first 16 bytes  

  EVP_CIPHER_CTX ctx;
  EVP_DecryptInit(&ctx, EVP_aes_256_cbc(), ENCRYPTION_KEY_256AES, encrypted_buffer);
  EVP_DecryptUpdate(&ctx, decrypted_buffer, &len, (unsigned char*)encrypted_buffer+16, encrypted_len >= 16 ? encrypted_len - 16 : encrypted_len);
  decrypted_len = len;
  EVP_DecryptFinal(&ctx, decrypted_buffer+len, &len);
  decrypted_len += len;

  decrypted_buffer[decrypted_len] = '\0';
  printf("Decrypted text: %s \n", decrypted_buffer);

  // Cleanup OpenSSL
  EVP_cleanup();
  ERR_free_strings();

  return 1;
}

// ******************* Password security RC128
// PDF spec clearly describes an algorithm how to get encryption key from entered password 
// it is different with every algorithm
// I skipped that part here and hardcoded encryption key here I got from open password ("open")
// In RC128 algorithm, every object is encrypted with different key. The Encryption key has to be
// adjusted/hashed with object ID to get proper "object encryption key"

// To ilustrate how to use OpenSSL to decrypt stream I'm decrypting stream ToUnicode from following dictionary
// the object ID is "17 0" here (major=17, minor=0)

/*
17 0 obj
<< / Length 295
/ Filter / FlateDecode
>>
stream
.....
endstream
endobj
*/

//download testing file from: https://drive.google.com/open?id=0B1ppJrJ4SN8EVXlWSmN0OUswTXM
#define FILE_NAME_128RC L"p:\\security\\secured 128RC.pdf"
static const unsigned char ENCRYPTION_KEY_128RC[16] = {
  0x64, 0xEE, 0x08, 0x6B, 0x83, 0x29, 0x2F, 0x0F, 0xFF, 0xD4, 0x52, 0x3E, 0xD8, 0xDB, 0x68, 0xAA
};
static int ENCRYPTION_KEY_LENGTH_128RC = 16;
#define TOUNICODE_STREAM_OFFSET 21451
#define TOUNICODE_STREAM_LENGTH 295
#define TOUNICODE_OBJ_ID_MAJOR 17
#define TOUNICODE_OBJ_ID_MINOR 0

int decrypt_file_128RC()
{

  unsigned char encrypted_buffer[1024];
  unsigned char decrypted_buffer[1024];
  unsigned char object_encryption_key[16];
  int encrypted_len = TOUNICODE_STREAM_LENGTH;
  int decrypted_len;
  int len;
  memset(decrypted_buffer, 0, 1024);

  // reading specific buffer from the pdf file
  read_string_from_file(FILE_NAME_128RC, TOUNICODE_STREAM_OFFSET, encrypted_len, encrypted_buffer, &decrypted_len);

  // going to compute Object encryption key by hashing general Encryption key with object IDs
  unsigned char obj_id[5];
  obj_id[0] = (unsigned char) TOUNICODE_OBJ_ID_MAJOR;
  obj_id[1] = (unsigned char)(TOUNICODE_OBJ_ID_MAJOR >> 8);
  obj_id[2] = (unsigned char)(TOUNICODE_OBJ_ID_MAJOR >> 16);
  obj_id[3] = (unsigned char) TOUNICODE_OBJ_ID_MINOR;
  obj_id[4] = (unsigned char)(TOUNICODE_OBJ_ID_MINOR >> 8);

  // MD5 produces the object encryption key from object ID+document encryption key
  MD5_CTX md5_ctx;
  MD5_Init(&md5_ctx);
  MD5_Update(&md5_ctx, ENCRYPTION_KEY_128RC, ENCRYPTION_KEY_LENGTH_128RC);
  MD5_Update(&md5_ctx, obj_id, 5);
  MD5_Final(object_encryption_key, &md5_ctx);

  EVP_CIPHER_CTX ctx;
  // key length for EVP_rc4 is 16bytes, so we are good here
  EVP_DecryptInit(&ctx, EVP_rc4(), object_encryption_key, NULL);
  EVP_DecryptUpdate(&ctx, decrypted_buffer, &len, encrypted_buffer, encrypted_len);
  decrypted_len = len;
  EVP_DecryptFinal(&ctx, decrypted_buffer + len, &len);
  decrypted_len += len;

  decrypted_buffer[decrypted_len] = '\0';
  printf("Decrypted stream: %s \n", decrypted_buffer);

  return 1;
}



int main()
{
  // signing file first
  sign_file();
  
  // decrypting string from AES256 file
  decrypt_file_256AES();
  return 0;
}
