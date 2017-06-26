#include <stdlib.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>

#include "mcpr/crypto.h"
#include "mcpr/mcpr.h"

ssize_t mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX *ctx_encrypt, size_t len) {
    if(len > INT_MAX) { mcpr_errno = MCPR_EINTERNAL; return -1; }

    int writtenlen;
    if(EVP_EncryptUpdate(ctx_encrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0) {
        mcpr_errno = MCPR_EUNKNOWN;
        return -1;
    }
    return writtenlen;
}

ssize_t mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX *ctx_decrypt, size_t len) {
    if(len > INT_MAX) { mcpr_errno = MCPR_EINTERNAL; return -1; }

    int writtenlen;
    if(EVP_DecryptUpdate(ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0) {
        mcpr_errno = MCPR_EUNKNOWN;
        return -1;
    }
    return writtenlen;
}
