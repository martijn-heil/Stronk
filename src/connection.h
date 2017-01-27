#ifndef STRONK_CONNECTION_H
#define STRONK_CONNECTION_H

#include <stdio.h>
#include <stdbool.h>

struct connection
{
    FILE *fp;
    bool use_encryption;

    enum mcpr_state state;
    bool use_compression;
    unsigned int compression_treshold; // Not guaranteed to be initialized if use_compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    EVP_CIPHER_CTX ctx_decrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false
};

#endif
