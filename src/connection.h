#ifndef STRONK_CONNECTION_H
#define STRONK_CONNECTION_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <openssl/evp.h>

struct connection
{
    int fd;
    bool use_encryption;

    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX *ctx_encrypt;  // Will be initialized to NULL. Should be free'd
    EVP_CIPHER_CTX *ctx_decrypt;  // Will be initialized to NULL. Should be free'd
    int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false
};

#endif
