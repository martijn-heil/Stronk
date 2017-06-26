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
#include <unistd.h>
#include <openssl/evp.h>
#include <ninio/bstream.h>
#include <ninio/ninio.h>
#include <mcpr/mcpr.h>
#include <mcpr/connection.h>
#include <mcpr/crypto.h>

#define BLOCK_SIZE EVP_CIPHER_block_size(EVP_aes_128_cfb8())


typedef void mcpr_connection; // temporary.. should be removed when actually compiling TODO
struct conn
{
    struct bstream *io_stream;
    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.
    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;
    EVP_CIPHER_CTX ctx_decrypt;
    unsigned int reference_count;
    struct ninio_buffer receiving_buf;
};


mcpr_connection *mcpr_connection_new(struct bstream *stream)
{
    struct conn *conn = malloc(sizeof(struct conn));
    if(conn == NULL) { return NULL; }

    conn->io_stream = stream;
    conn->state = MCPR_STATE_HANDSHAKE;
    conn->use_compression = false;
    conn->use_encryption = false;
    conn->reference_count = 1;

    conn->receiving_buf.content = malloc(32 * BLOCK_SIZE);
    if(conn->receiving_buf.content == NULL) { free(conn); return NULL; }
    conn->receiving_buf.max_size = 32 * BLOCK_SIZE;
    conn->receiving_buf.size = 0;

    return conn;
}

void mcpr_connection_incref(mcpr_connection *conn)
{
    ((struct conn *) conn)->reference_count++;
}

void mcpr_connection_decref(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->reference_count--;
    if(conn->reference_count <= 0)
    {
        bstream_decref(conn->io_stream);
        free(conn->receiving_buf.content);
        free(conn);
    }
}

bool update_receiving_buffer(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;

    bool again = false;
    do {
        // Ensure the buffer is large enough.
        if(conn->receiving_buf.max_size < conn->receiving_buf.size + BLOCK_SIZE) {
            void *tmp = realloc(conn->receiving_buf.content, conn->receiving_buf.size + BLOCK_SIZE * 32);
            if(tmp == NULL) { return false; }
            conn->receiving_buf.content = tmp;
        }

        // Read a single block.
        void *tmpbuf = malloc(BLOCK_SIZE);
        if(tmpbuf == NULL) { return false; }
        bool result = bstream_read(conn->io_stream, tmpbuf, BLOCK_SIZE);
        if(!result) { free(tmpbuf); return false; }

        ssize_t decrypt_result = mcpr_crypto_decrypt(conn->receiving_buf.content + conn->receiving_buf.size, tmpbuf, &(conn->ctx_decrypt), BLOCK_SIZE);
        if(decrypt_result == -1) { free(tmpbuf); return false; }
        conn->receiving_buf.size += decrypt_result;
    } while(again);

    return true;
}

void mcpr_connection_update(mcpr_connection *tmpconn)
{
    update_receiving_buffer(tmpconn);
}

static bool mcpr_connection_write(mcpr_connection *tmpconn, const void *in, size_t bytes)
{
    struct conn *conn = (struct conn *) tmpconn;

    if(conn->use_encryption)
    {
        void *encrypted_data = malloc(bytes + BLOCK_SIZE - 1);
        if(encrypted_data == NULL) return false;

        ssize_t bytes_written = mcpr_encrypt(encrypted_data, in, conn->ctx_encrypt, bytes);
        if(bytes_written == -1) { free(encrypted_data); return false; }

        bool result = bstream_write(conn->io_stream, encrypted_data, bytes_written);
        free(encrypted_data);
        return result;
    }
    else
    {
        return bstream_write(conn->io_stream, in, bytes);
    }
}

#endif
