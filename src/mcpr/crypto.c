/*
  MIT License

  Copyright (c) 2016-2020 Martijn Heil

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>

#include <ninerr/ninerr.h>

#include "mcpr/crypto.h"
#include "mcpr/mcpr.h"

#include "internal.h"



ssize_t mcpr_crypto_encrypt(void *out, const void *data, EVP_CIPHER_CTX *ctx_encrypt, size_t len) {
  if(len > INT_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }

  int writtenlen;
  if(EVP_EncryptUpdate(ctx_encrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0) {
    ninerr_set_err(ninerr_new("EVP_EncryptUpdate failed.", false));
    return -1;
  }
  return writtenlen;
}

ssize_t mcpr_crypto_decrypt(void *out, const void *data, EVP_CIPHER_CTX *ctx_decrypt, size_t len) {
  if(len > INT_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }

  int writtenlen;
  if(EVP_DecryptUpdate(ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0) {
    ninerr_set_err(ninerr_new("EVP_DecryptUpdate failed.", false));
    return -1;
  }
  return writtenlen;
}

void mcpr_crypto_stringify_sha1(char *out, const void *hash)
{
  DEBUG_PRINT("in mcpr_crypto_stringify_sha1(out = %p, hash = %p)", (void *) out, (void *) hash);
  const unsigned char *bytes = (const unsigned char *) hash;
  bool is_negative = bytes[19] & 0x80;
  if(is_negative) *out = '-';

  char *outp = (is_negative) ? out + 1 : out;

  const unsigned char *final_bytes;
  if(is_negative)
  {
    unsigned char tmp[20];
    bool carry = true;
    unsigned char new_byte;
    unsigned char value;
    for(int_fast8_t i = 19; i >= 0; i--)
    {
      new_byte = ~(bytes[i]) & 0xFF;
      if(carry)
      {
        carry = new_byte == 0xFF;
        tmp[i] = new_byte + 1;
      }
      else
      {
        tmp[i] = new_byte;
      }
    }
    final_bytes = tmp;
  }
  else
  {
    final_bytes = bytes;
  }

  // Write it as a hex string.
  for(uint_fast8_t i = 0; i < 20; i++)
  {
    sprintf(outp, "%02x", final_bytes[i]);
    outp += 2;
  }
  *outp = '\0';
  // Trim leading zeros
  outp = (is_negative) ? out + 1 : out;
  for(uint_fast8_t i = 0; i < 40; i++)
  {
    if(*outp != '0') break;
    outp++;
  }
}
