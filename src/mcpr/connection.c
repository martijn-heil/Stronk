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
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <ninio/bstream.h>
#include <ninio/ninio.h>
#include <mcpr/mcpr.h>
#include <mcpr/connection.h>
#include <mcpr/crypto.h>
#include <mcpr/abstract_packet.h>
#include <mcpr/codec.h>
#include <util.h>

#define BLOCK_SIZE EVP_CIPHER_block_size(EVP_aes_128_cfb8())

typedef void mcpr_connection; // temporary.. should be removed when actually compiling TODO
struct conn
{
    bool is_closed;
    struct bstream *io_stream;
    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.
    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;
    EVP_CIPHER_CTX ctx_decrypt;
    unsigned int reference_count;
    struct ninio_buffer receiving_buf;
    void (*packet_handler)(const struct mcpr_abstract_packet *pkt);

    bool tmp_encryption_state_available;
    RSA *rsa;
    int32_t verify_token_length;
    uint8_t *verify_token;
};

bool mcpr_connection_write_packet(mcpr_connection *conn, const struct mcpr_abstract_packet *pkt);

static void mcpr_connection_close(struct conn *conn, const char *reason) {
    if(conn->state == MCPR_STATE_LOGIN || conn->state == MCPR_STATE_PLAY) {
        struct mcpr_abstract_packet pkt;
        if(conn->state == MCPR_STATE_LOGIN) {
            pkt.id = MCPR_PKT_LG_CB_DISCONNECT;
            pkt.data.login.clientbound.disconnect.reason = (reason != NULL) ? reason : "disconnected";
        } else if(conn->state == MCPR_STATE_PLAY) {
            pkt.id = MCPR_PKT_PL_CB_DISCONNECT;
            pkt.data.play.clientbound.disconnect.reason = (reason != NULL) ? reason : "disconnected";
        }

        mcpr_connection_write_packet(conn, &pkt);
    }

    conn->is_closed = true;
}


mcpr_connection *mcpr_connection_new(struct bstream *stream)
{
    struct conn *conn = malloc(sizeof(struct conn));
    if(conn == NULL) { return NULL; }
    bstream_incref(stream);

    conn->io_stream = stream;
    conn->state = MCPR_STATE_HANDSHAKE;
    conn->use_compression = false;
    conn->use_encryption = false;
    conn->reference_count = 1;

    conn->receiving_buf.content = malloc(32 * BLOCK_SIZE);
    if(conn->receiving_buf.content == NULL) { free(conn); return NULL; }
    conn->receiving_buf.max_size = 32 * BLOCK_SIZE;
    conn->receiving_buf.size = 0;
    conn->packet_handler = NULL;

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
        mcpr_connection_close(conn, NULL);
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

bool mcpr_connection_update(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;
    if(!update_receiving_buffer(tmpconn)) return false;

    while(true) {
        int32_t pktlen;
        ssize_t result = mcpr_decode_varint(&pktlen, conn->receiving_buf.content, conn->receiving_buf.size);
        if(result == -1) return true;
        if((conn->receiving_buf.size - 5) >= pktlen) {
            struct mcpr_abstract_packet *pkt = mcpr_decode_abstract_packet(conn->receiving_buf.content, conn->receiving_buf.size);
            if(pkt == NULL) return false;
            conn->packet_handler(pkt);
            free(pkt);
            memmove(conn->receiving_buf.content, conn->receiving_buf.content + pktlen + 5, conn->receiving_buf.size - pktlen - 5);
        }
    }

    return true;
}

static bool mcpr_connection_write(mcpr_connection *tmpconn, const void *in, size_t bytes)
{
    struct conn *conn = (struct conn *) tmpconn;

    if(conn->use_encryption)
    {
        void *encrypted_data = malloc(bytes + BLOCK_SIZE - 1);
        if(encrypted_data == NULL) return false;

        ssize_t bytes_written = mcpr_crypto_encrypt(encrypted_data, in, &(conn->ctx_encrypt), bytes);
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

bool mcpr_connection_write_packet(mcpr_connection *tmpconn, const struct mcpr_abstract_packet *pkt) {
    void *buf;
    ssize_t result = mcpr_encode_abstract_packet(&buf, pkt);
    if(result == -1) return false;
    if(!mcpr_connection_write(tmpconn, buf, result)) { free(buf); return false; }
    free(buf);
    return true;
}

void mcpr_connection_set_packet_handler(mcpr_connection *tmpconn, void (*on_packet)(const struct mcpr_abstract_packet *pkt)) {
    struct conn *conn = (struct conn *) tmpconn;
    conn->packet_handler = on_packet;
}

void mcpr_connection_set_crypto(mcpr_connection *tmpconn, EVP_CIPHER_CTX ctx_encrypt, EVP_CIPHER_CTX ctx_decrypt)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->ctx_encrypt = ctx_encrypt;
    conn->ctx_decrypt = ctx_decrypt;
}

void mcpr_connection_set_compression(mcpr_connection *tmpconn, bool compression)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->use_compression = compression;
}

#endif
