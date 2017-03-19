/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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

#ifndef MCPR_CONNECTION_H
#define MCPR_CONNECTION_H

#include <stdlib.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <ninio/bstream.h>
#include <mcpr/mcpr.h>
#include <mcpr/connection.h>

typedef void* mcpr_connection;
struct conn
{
    int sockfd; // Not advised to read/write to the socket. May result in unwanted behaviour
    struct bstream bstream;
    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.
    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;
    EVP_CIPHER_CTX ctx_decrypt;
    int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false
};

mcpr_connection *mcpr_connection_new(int sockfd)
{
    struct conn *conn = malloc(sizeof(struct conn));
    if(conn == NULL) return NULL;
    conn->sockfd = sockfd;
    conn->state = MCPR_STATE_HANDSHAKE;
    conn->use_compression = false;
    conn->use_encryption = false;
}

void mcpr_connection_delete(mcpr_connection *conn)
{

}

struct bstream mcpr_connection_to_bstream(mcpr_connection *conn)
{
    return ((struct conn *) *conn)->bstream;
}

#endif
