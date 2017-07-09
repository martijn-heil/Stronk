#include <stdlib.h>
#include <stdbool.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>

#include <ninerr/ninerr.h>

#include "mcpr/crypto.h"
#include "mcpr/mcpr.h"



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

ssize_t mcpr_crypto_stringify_sha1(char *out, const void *hash2)
{
    const unsigned char *hash1 = (const unsigned char *) hash2;

    unsigned char uhash[SHA_DIGEST_LENGTH];
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        uhash[i] = hash1[i];
    }

    char *hash = (char *) uhash;

    bool is_negative = *hash < 0;
    if(is_negative) {
        bool carry = true;
        int i;
        unsigned char new_byte;
        unsigned char value;

        for(i = SHA_DIGEST_LENGTH - 1; i >= 0; --i) {
            value = uhash[i];
            new_byte = ~value & 0xFF;
            if(carry) {
                carry = new_byte == 0xFF;
                uhash[i] = new_byte + 1;
            } else {
                uhash[i] = new_byte;
            }
        }
    }

    char *stringified_hashp;
    if(is_negative) // Hash is negative.
    {
        *out = '-';
        stringified_hashp = out + 1;
    }
    else
    {
        stringified_hashp = out;
    }

    // Write it as a hex string.
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(stringified_hashp, "%02x", uhash[i]);
        stringified_hashp += 2;
    }
    *stringified_hashp = '\0';

    // Trim leading zeros
    stringified_hashp = out + 1;
    for(int i = 0; i < SHA_DIGEST_LENGTH * 2; i++) {
        if(*stringified_hashp != '0') { break; }
        stringified_hashp++;
    }


    return 0;
}
