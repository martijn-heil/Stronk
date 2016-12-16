/*
    MIT License

    Copyright (c) 2016 Martijn Heil

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



    mcpr.c - Minecraft Protocol functions
*/

#ifndef MCPR_H
#define MCPR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>

#include <jansson/jansson.h>
#include <nbt/nbt.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

/*
    Minecraft Protocol (http://wiki.vg/Protocol)
    MCPR = MineCraft PRotocol.
*/

#define MCPR_PROTOCOL_VERSION 315


unsigned int *mcpr_get_errno();
/**
 * mapi_errno is guaranteed to be thread local.
 */
#define mcpr_errno (*mcpr_get_errno())


/**
 * Compress data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least the size of max_out_size. May not be NULL.
 * @param [in] in Input buffer, should be at least the size of in_size.
 * @param in_size Amount of bytes to read from in.
 * @param max_out_size Maximum amount of bytes to write to out.
 *
 * @returns The new size of out or -1 upon error.
 */
ssize_t mcpr_decompress(void *out, const void *in, size_t max_out_size, size_t in_size);

/**
 * Out should be at least the size of mcr_compress_bounds(n)
 *
 * Returns the new size of out or -1 upon error.
 */
ssize_t mcpr_compress(void *out, const void *in, size_t n);

/**
 * Calculate maximum compressed size for len amount of bytes.
 *
 * @returns The maximum compressed size for len amount of bytes.
 */
size_t mcpr_compress_bounds(size_t len);


enum mcpr_state {
    MCPR_STATE_HANDSHAKE = 0,
    MCPR_STATE_STATUS = 1,
    MCPR_STATE_LOGIN = 2,
    MCPR_STATE_PLAY = 3
};

enum mcpr_connection_type {
    MCPR_CONNECTION_TYPE_SERVERBOUND,
    MCPR_CONNECTION_TYPE_CLIENTBOUND
};

struct mcpr_packet {
    uint8_t id;
    void *data;
    size_t data_len;
};


/**
 * Encrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least the size of (len + mcpr_client->encryption_block_size -1), may not be NULL.
 * @param [in] data Raw binary data to encrypt, should be at least the size of len.
 * @param [in] ctx_encrypt ctx_encrypt from the mcpr_client struct.
 *
 * @returns Negative integer upon error.
 */
int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len);

/**
 * Decrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least size of (len + mcpr_client->encryption_block_size), may not be NULL.
 * @param [in] data Input buffer, should be at least the size of len, may not be NULL.
 *
 * @returns Negative integer upon error.
 */
int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len);

ssize_t mcpr_encode_packet(void *out, struct mcpr_packet *pkt, bool use_compression, bool use_encryption);

#endif
