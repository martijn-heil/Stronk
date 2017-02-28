#include <stdlib.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>

#include "mcpr/crypto.h"

int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len) {
    if(len > INT_MAX) { return -1; }

    int writtenlen;
    if(EVP_EncryptUpdate(&ctx_encrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0) {
        return -1;
    }
    return writtenlen;
}

int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len) {
    if(len > INT_MAX) { return -1; }

    int writtenlen;
    if(EVP_DecryptUpdate(&ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0) {
        return -1;
    }
    return writtenlen;
}
