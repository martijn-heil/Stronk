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

/*
    mcpr connection handles easy reading & writing of packets, mainly for the encryption and compression.
    Due to how the Minecraft protocol works, we need some buffering for this,
    which would be annoying to have to implement yourself everytime.
    A mcpr connection either be client side or server side.
*/

#ifndef MCPR_CONNECTION_H
#define MCPR_CONNECTION_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <ninio/bstream.h>
#include <ninio/ninio.h>
#include <ninerr/ninerr.h>
#include <mcpr/mcpr.h>
#include <mcpr/connection.h>
#include <mcpr/crypto.h>
#include <mcpr/packet.h>
#include <mcpr/codec.h>
#include <util.h>

#include "internal.h"

#define BLOCK_SIZE EVP_CIPHER_block_size(EVP_aes_128_cfb8())

typedef void mcpr_connection;

// TODO COMPRESSION!!!
struct conn
{
    bool is_closed;
    struct bstream *io_stream;
    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.
    bool use_encryption;
    EVP_CIPHER_CTX *ctx_encrypt;
    EVP_CIPHER_CTX *ctx_decrypt;
    unsigned int reference_count;
    struct ninio_buffer receiving_buf;
    bool (*packet_handler)(const struct mcpr_packet *pkt, mcpr_connection *conn);

    bool tmp_encryption_state_available;
    RSA *rsa;
    int32_t verify_token_length;
    uint8_t *verify_token;
};

bool mcpr_connection_write_packet(mcpr_connection *conn, const struct mcpr_packet *pkt);

enum mcpr_state mcpr_connection_get_state(mcpr_connection *conn)
{
    return ((struct conn *) conn)->state;
}

void mcpr_connection_close(mcpr_connection *tmpconn, const char *reason)
{
    struct conn *conn = (struct conn *) tmpconn;
    if(!conn->is_closed)
    {
        if(conn->state == MCPR_STATE_LOGIN || conn->state == MCPR_STATE_PLAY) {
            struct mcpr_packet pkt;
            pkt.id = MCPR_PKT_PL_CB_DISCONNECT;
            pkt.state = (conn->state == MCPR_STATE_PLAY) ? MCPR_STATE_PLAY : MCPR_STATE_LOGIN;
            IGNORE("-Wdiscarded-qualifiers")
            pkt.data.play.clientbound.disconnect.reason = (reason != NULL) ? reason : "{\"text\":\"Disconnected by server.\"}";
            END_IGNORE()
            mcpr_connection_write_packet(conn, &pkt);
        }

        conn->is_closed = true;
    }
}


mcpr_connection *mcpr_connection_new(struct bstream *stream)
{
    struct conn *conn = malloc(sizeof(struct conn));
    if(conn == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }
    bstream_incref(stream);

    conn->io_stream = stream;
    conn->state = MCPR_STATE_HANDSHAKE;
    conn->use_compression = false;
    conn->use_encryption = false;
    conn->reference_count = 1;

    conn->ctx_encrypt = NULL;
    conn->ctx_decrypt = NULL;

    conn->receiving_buf.content = malloc(256 * BLOCK_SIZE);
    if(conn->receiving_buf.content == NULL) { free(conn); ninerr_set_err(ninerr_from_errno()); return NULL; }
    conn->receiving_buf.max_size = 256 * BLOCK_SIZE;
    conn->receiving_buf.size = 0;
    conn->packet_handler = NULL;

    conn->is_closed = false;

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
        mcpr_connection_close(conn, NULL);
        EVP_CIPHER_CTX_free(conn->ctx_encrypt);
        EVP_CIPHER_CTX_free(conn->ctx_decrypt);
        free(conn->receiving_buf.content);
        free(conn);
    }
}

void mcpr_connection_set_use_encryption(mcpr_connection *conn, bool value)
{
    ((struct conn *) conn)->use_encryption = value;
}

// TODO BLOCK_SIZE is apparentely only 1, horribly inefficient
bool update_receiving_buffer(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;

    again:
        // Ensure the buffer is large enough.
        if(conn->receiving_buf.max_size < conn->receiving_buf.size + BLOCK_SIZE)
        {
            void *tmp = realloc(conn->receiving_buf.content, conn->receiving_buf.max_size + BLOCK_SIZE * 256);
            if(tmp == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
            conn->receiving_buf.max_size += BLOCK_SIZE * 256;
            conn->receiving_buf.content = tmp;
        }

        // Read a single block.

        if(conn->use_encryption)
        {
            void *tmpbuf = malloc(BLOCK_SIZE);
            if(tmpbuf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
            bool result = bstream_read(conn->io_stream, tmpbuf, BLOCK_SIZE);
            if(!result)
            {
                if(ninerr != NULL && strcmp(ninerr->type, "ninerr_wouldblock") == 0)
                {
                    return true;
                }
                else if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
                {
                    free(tmpbuf);
                    mcpr_connection_close(tmpconn, NULL);
                    return false;
                }
            }

            ssize_t decrypt_result = mcpr_crypto_decrypt(conn->receiving_buf.content + conn->receiving_buf.size, tmpbuf, conn->ctx_decrypt, BLOCK_SIZE);
            if(decrypt_result == -1) { free(tmpbuf); return false; }
            conn->receiving_buf.size += decrypt_result;
            goto again;
        }
        else
        {
            ssize_t result = bstream_read_max(conn->io_stream, conn->receiving_buf.content + conn->receiving_buf.size, 256);
            if(result < 0)
            {
                if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
                {
                    mcpr_connection_close(tmpconn, NULL);
                    return false;
                }
                else
                {
                    return false;
                }
            }
            conn->receiving_buf.size += result;
            if(result > 0) goto again;
        }

    return true;
}

bool mcpr_connection_update(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;
    if(!update_receiving_buffer(tmpconn)) return false;

    while(conn->receiving_buf.size > 0)
    {
        int32_t pktlen;
        ssize_t result = mcpr_decode_varint(&pktlen, conn->receiving_buf.content, conn->receiving_buf.size);
        if(result == -1) return true;
        if(pktlen <= 0) { ninerr_set_err(ninerr_new("Received invalid packet length")); return false; }
        if((conn->receiving_buf.size - result) >= (uint32_t) pktlen)
        {
            DEBUG_PRINT("Received packet in mcpr_connection_update\n");
            struct mcpr_packet *pkt;
            ssize_t bytes_read = mcpr_decode_packet(&pkt, conn->receiving_buf.content + result, conn->state, (size_t) pktlen);
            if(bytes_read == -1) { mcpr_connection_close(tmpconn, NULL); return false; }
            memmove(conn->receiving_buf.content, conn->receiving_buf.content + bytes_read + result, conn->receiving_buf.size - bytes_read - result);
            conn->receiving_buf.size -= bytes_read + result;
            bool conn_still_valid = conn->packet_handler(pkt, conn);
            free(pkt);
            if(!conn_still_valid) return true;
        }
        else
        {
            return true;
        }
    }

    return true;
}

bool mcpr_connection_write(mcpr_connection *tmpconn, const void *in, size_t bytes)
{
    DEBUG_PRINT("Writing %zu bytes to mcpr_connection at address %p", bytes, (void *) tmpconn)
    struct conn *conn = (struct conn *) tmpconn;

    if(conn->use_encryption)
    {
        DEBUG_PRINT("Writing those bytes encrypted.");
        void *encrypted_data = malloc(bytes + BLOCK_SIZE - 1);
        if(encrypted_data == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }

        ssize_t encrypted_data_length = mcpr_crypto_encrypt(encrypted_data, in, conn->ctx_encrypt, bytes);
        if(encrypted_data_length == -1) { free(encrypted_data); return false; }

        if(conn->use_compression) // TODO compression treshold
        {
            DEBUG_PRINT("Writing those bytes compressed.");
            void *compressed_data = malloc(mcpr_compress_bounds(encrypted_data_length));
            if(compressed_data == NULL) { free(encrypted_data); ninerr_set_err(ninerr_from_errno()); return false; }
            ssize_t compression_result = mcpr_compress(compressed_data, encrypted_data, encrypted_data_length);
            if(compression_result == -1) { free(encrypted_data); free(compressed_data); return false; }

            bool write_result = bstream_write(conn->io_stream, compressed_data, compression_result);
            free(encrypted_data);
            free(compressed_data);
            return write_result;
        }
        else
        {
            DEBUG_PRINT("Writing those bytes uncompressed.");
            bool write_result = bstream_write(conn->io_stream, encrypted_data, encrypted_data_length);
            free(encrypted_data);
            return write_result;
        }
    }
    else
    {
        DEBUG_PRINT("Writing those bytes unencrypted.");
        if(conn->use_compression)
        {
            DEBUG_PRINT("Writing those bytes compressed.");
            void *compressed_data = malloc(mcpr_compress_bounds(bytes));
            if(compressed_data == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }

            ssize_t compression_result = mcpr_compress(compressed_data, in, bytes);
            if(compression_result == -1) { free(compressed_data); return false; }
            bool write_result = bstream_write(conn->io_stream, compressed_data, compression_result);
            free(compressed_data);
            return write_result;
        }
        else
        {
            DEBUG_PRINT("Writing those bytes uncompressed.");
            bool result = bstream_write(conn->io_stream, in, bytes);
            return result;
        }
    }
}

bool mcpr_connection_write_packet(mcpr_connection *tmpconn, const struct mcpr_packet *pkt)
{
    DEBUG_PRINT("Writing packet (numerical ID: 0x%02x, state: %s) to mcpr_connection at address %p\n", mcpr_packet_type_to_byte(pkt->id), mcpr_state_to_string(pkt->state), tmpconn);
    size_t required_size = MCPR_VARINT_SIZE_MAX + mcpr_encode_packet_bounds(pkt);
    void *buf = malloc(required_size);
    if(buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
    size_t pktlen = mcpr_encode_packet(buf + MCPR_VARINT_SIZE_MAX, pkt);
    if(pktlen == 0) { free(buf); return false; }
    if(pktlen > INT32_MAX) { free(buf); ninerr_set_err(ninerr_arithmetic_new()); return false; }

    size_t skip = MCPR_VARINT_SIZE_MAX - mcpr_varint_bounds((int32_t) pktlen);
    size_t bytes_written_1 = mcpr_encode_varint(buf + skip, (int32_t) pktlen);
    DEBUG_PRINT("Writing packet, total size: %zu, size before prefixed length: %zu\n", (size_t) (pktlen + bytes_written_1), pktlen);
    if(!mcpr_connection_write(tmpconn, buf + skip, pktlen + bytes_written_1)) { free(buf); return false; }
    free(buf);
    return true;
}

void mcpr_connection_set_packet_handler(mcpr_connection *tmpconn, bool (*on_packet)(const struct mcpr_packet *pkt, mcpr_connection *conn))
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->packet_handler = on_packet;
}

void mcpr_connection_set_crypto(mcpr_connection *tmpconn, EVP_CIPHER_CTX *ctx_encrypt, EVP_CIPHER_CTX *ctx_decrypt)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->ctx_encrypt = ctx_encrypt;
    conn->ctx_decrypt = ctx_decrypt;
}

void mcpr_connection_set_state(mcpr_connection *conn, enum mcpr_state state)
{
    ((struct conn *) conn)->state = state;
}


void mcpr_connection_set_compression(mcpr_connection *tmpconn, bool compression)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->use_compression = compression;
}

bool mcpr_connection_is_closed(mcpr_connection *conn)
{
    return ((struct conn *) conn)->is_closed;
}


#endif
